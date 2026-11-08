# Thiết kế Slab Allocator một size class

Tài liệu này mô tả implementation hiện tại trong `kernel/slab.c`. Đây là một cache
cho object có kích thước tối đa 64 byte, dùng trang 4 KiB do page allocator của
xv6 cung cấp. Các trang slab dùng một freelist chung được bảo vệ bởi một spinlock;
per-CPU cache, nhiều size class và thu hồi trang slab chưa được triển khai.

## Bố trí một trang slab

Mỗi trang bắt đầu bằng header 32 byte. Phần còn lại chứa 63 slot, mỗi slot có
stride 64 byte:

```text
Một trang vật lý: 4096 bytes
┌──────────────────────────────────────────────────────────────────────────┐
│ Header: 32 bytes                                                        │
├──────────────────┬──────────────────┬─────────────────────┬─────────────┤
│ Slot 0: 64 bytes  │ Slot 1: 64 bytes │ ... Slot 62: 64 bytes│ 32B dư cuối │
└──────────────────┴──────────────────┴─────────────────────┴─────────────┘
```

Header `struct slab_header` trong mã gồm:

| Trường | Kích thước | Vai trò |
|---|---:|---|
| `magic` | 4 bytes | Dấu nhận diện trang slab (`SLAB_MAGIC`) |
| `obj_size` | 2 bytes | Stride của slot, hiện là 64 bytes |
| `capacity` | 2 bytes | Tổng số slot, hiện là 63 |
| `free_count` | 2 bytes | Số slot trống trên trang |
| `_pad` | 2 bytes | Đệm tường minh |
| `_reserved` | 4 bytes | Dự phòng |
| `freelist` | 8 bytes | Slot trống đầu tiên của trang |
| `next` | 8 bytes | Trang kế tiếp trong danh sách của cache |

Tổng kích thước là 32 bytes. `struct slab_slot` chỉ chứa một con trỏ `next`;
khi slot đang trống, con trỏ này được lưu ngay trong 8 byte đầu của slot. Khi
slot được cấp phát, vùng đó thuộc về dữ liệu object và không còn là freelist.

## Alignment và capacity

Trang do `kalloc()` trả về căn theo `PGSIZE` (4096 bytes). Header dài 32 bytes,
do đó vùng slot bắt đầu ở địa chỉ căn 8 byte. `kmalloc_init()` chọn class 64
bytes và căn stride lên bội số 8:

```text
stride   = ALIGN_UP(max(64, sizeof(struct slab_slot)), 8) = 64 bytes
capacity = (PGSIZE - sizeof(struct slab_header)) / stride
         = (4096 - 32) / 64 = 63 slots
tail     = 4096 - 32 - 63 * 64 = 32 bytes
```

Phần dư 32 byte cuối trang không tạo thành slot. Một yêu cầu có size từ 1 đến
64 byte nhận một slot 64 byte; size 0 hoặc lớn hơn 64 bị từ chối.

## Khởi tạo allocator và trang slab

Sau khi `kinit()` tạo page allocator, `main()` gọi `kmalloc_init()` đúng một lần
để khởi tạo spinlock và cấu hình size class/capacity. Sau đó `slab_test()` chạy
trên cache đã khởi tạo; nó không tự khởi tạo lại allocator.

Khi cache cần slot nhưng chưa có slab còn chỗ, `slab_page_create()` gọi
`kalloc()` để lấy trang. Hàm thiết lập header rồi duyệt các slot để nối chúng
thành embedded freelist. Cache liên kết trang mới vào đầu danh sách `slabs`.
Nếu `kalloc()` hết trang, trang mới không được tạo.

## Allocate và free

`kmalloc(size)` trả về `0` khi size không hợp lệ hoặc không lấy được trang do
OOM. Nếu hợp lệ, hàm lấy lock cache, tìm trang còn slot trống hoặc tạo trang
mới, lấy phần tử đầu freelist và giảm `free_count`. Sau khi nhả lock, toàn bộ
slot 64 byte được zero trước khi trả về cho caller.

`kmfree(NULL)` là no-op. Với con trỏ khác NULL, hàm kiểm tra địa chỉ thuộc vùng
RAM hợp lệ, header của trang có magic/size hợp lệ, con trỏ nằm đúng đầu một
slot, và slot chưa có trong freelist. Con trỏ không hợp lệ, double-free hoặc
counter không hợp lệ dẫn tới `panic()`. Slot được ghi mẫu byte `1`, nối vào đầu
freelist của trang và tăng `free_count`.

Các trang slab hiện không được trả lại page allocator khi rỗng. Kiểm tra
double-free duyệt freelist nên thời gian free phụ thuộc số slot trống trên
trang; không khẳng định mọi thao tác allocator là O(1).

## Tự kiểm thử lúc khởi động

`main()` gọi `kmalloc_init()` và ngay sau đó `slab_test()`, sau khi `kinit()` đã
hoàn tất. Test kiểm tra size biên và `kmfree(NULL)`, căn lề 8 byte, LIFO reuse,
ghi/đọc pattern trên slab thứ nhất, cấp đủ slot để buộc tạo slab thứ hai, và
giải phóng các object sau kiểm tra.

Để thử lỗi cấp trang mà không làm cạn RAM thật, boot test bật một cờ nội bộ để
lần tạo slab tiếp theo trả về OOM. Nó xác nhận `kmalloc()` trả về `0`, sau đó
tắt cờ và retry để xác nhận lock đã được nhả và cấp phát lại thành công. Đây là
fault injection cho nhánh xử lý OOM, không phải kiểm tra cạn toàn bộ RAM.

Mặc định build chạy bộ test bình thường. Các ca kết thúc có chủ đích bằng panic
có thể chạy riêng bằng `SLAB_TEST_CASE`:

| Giá trị | Ca kiểm thử | Panic mong đợi |
|---:|---|---|
| `1` | Double-free cùng object | `kmfree: double free detected` |
| `2` | Free con trỏ lệch 1 byte trong object | `kmfree: unaligned slot pointer` |
| `3` | Free trang thô không phải slab | `kmfree: invalid slab magic or corrupted header` |

Mỗi ca chạy toàn bộ test bình thường trước, in dòng `ALL PASSED`, sau đó chạy
ca âm riêng và phải panic ở thông báo tương ứng. Build một ca riêng bằng:

```sh
make clean
make TOOLPREFIX=riscv64-linux-gnu- SLAB_TEST_CASE=1 kernel/kernel fs.img
make TOOLPREFIX=riscv64-linux-gnu- qemu
```

Đổi `SLAB_TEST_CASE` thành `2` hoặc `3` để chạy ca khác. Xác nhận từng kết quả
panic rồi build lại chế độ mặc định trước khi chạy các bài test khác.

Đã kiểm tra local ngày 31/10/2026 sau `make clean`:

| Ca | Expected | Actual |
|---|---|---|
| Normal boot | `slab_test: ALL PASSED (capacity=63, obj_size=64)` | Đúng; QEMU tiếp tục tới shell |
| `SLAB_TEST_CASE=1` | `panic: kmfree: double free detected` | Đúng |
| `SLAB_TEST_CASE=2` | `panic: kmfree: unaligned slot pointer` | Đúng |
| `SLAB_TEST_CASE=3` | `panic: kmfree: invalid slab magic or corrupted header` | Đúng |

Nếu mọi điều kiện bình thường qua, console in:

```text
slab_test: ALL PASSED (capacity=63, obj_size=64)
```

Đây là test khởi động cho slab; `usertests` và `crash` của xv6 không thay thế
cho test này.