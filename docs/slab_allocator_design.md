 Đặc Tả Thiết Kế Bộ Cấp Phát Slab Hai Class (`S01` -> `S02` -> `S03a` -> `S04a`)

- **Phạm vi mã nguồn:** `kernel/slab.c`, `kernel/defs.h`, `kernel/main.c`
- **Các mốc hoàn thiện:** `S01` (#13), `S02` (#18 / PR #23), `S03a` (#27), `S04a` (#38 / PR #42)

---

## 1. Kiến Trúc Tổng Quan & Tương Tác Với Tầng Page (`kalloc` / `kfree`)

Bộ cấp phát đối tượng Slab (`kmalloc` / `kmfree`) hoạt động như một tầng đệm (object cache) bên trên bộ cấp phát trang vật lý `4096 B` (`PGSIZE`) của `xv6-riscv`:
- Khi một size class cần thêm vùng nhớ, tầng Slab gọi `kalloc()` để nhận một trang vật lý `4096 B` đã căn lề trang.
- Khi thu hồi đối tượng qua `kmfree(void *ptr)`, tầng Slab dùng macro `PGROUNDDOWN((uint64)ptr)` để dò ngược về địa chỉ đầu trang (`struct slab_header *`) trong thời gian $O(1)$ mà không cần người gọi truyền lại kích thước `size`.
- **Chính sách giữ trang rỗng (Empty Slab Retention):** Các trang Slab sau khi giải phóng hết đối tượng (`free_count == capacity`) được giữ lại trong danh sách `c->slabs` của class đó để tái sử dụng ngay cho các đợt cấp phát tiếp theo (không tính là rò rỉ bộ nhớ).

---

## 2. Hợp Đồng API & Bảng Phân Lớp Kích Thước (`64 B` và `128 B`)

### 2.1. Giao diện hàm (API Contract)
- `void kmalloc_init(void);` — Khởi tạo cấu hình và `spinlock` riêng cho từng size class (`slab_c64`, `slab_c128`).
- `void *kmalloc(uint64 size);` — Cấp phát 1 đối tượng thuộc class nhỏ nhất đủ chứa `size`.
- `void kmfree(void *ptr);` — Giải phóng đối tượng về lại trang Slab sở hữu (`kmfree(0)` là `no-op`).

### 2.2. Bảng phân lớp kích thước (`S03a`)

| Kích thước yêu cầu (`size`) | Class được chọn | `class_idx` | `stride` (`obj_size`) | Công thức `capacity` | `capacity` thực tế | Phần dư cuối trang (`tail_padding`) | Hành vi trả về |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :--- |
| `size == 0` | Không hợp lệ | — | — | — | — | — | Trả về `0` (`NULL`) |
| **`1 .. 64 B`** | **`class64`** | `0` | `64 B` | `(4096 - 32) / 64` | **`63` slots** | `32 B` | Con trỏ slot `64 B` (zero-filled) |
| **`65 .. 128 B`** | **`class128`** | `1` | `128 B` | `(4096 - 32) / 128` | **`31` slots** | `96 B` | Con trỏ slot `128 B` (zero-filled) |
| `size > 128 B` (`129..`) | Vượt ngưỡng | — | — | — | — | — | Trả về `0` (`NULL`) |

---

## 3. Bố Trí Bộ Nhớ (Memory Layout) & Quy Ước Fill (`0` / `1`)

### 3.1. Cấu trúc `struct slab_header` (`32 Bytes` cố định ở đầu mỗi trang Slab)
```c
struct slab_slot {
  struct slab_slot *next; // 8 B nhúng ở đầu mỗi slot đang rảnh
};

struct slab_header {
  uint32 magic;               // 4 B: 0x534C4142U ("SLAB")
  uint16 obj_size;            // 2 B: Bước nhảy slot (64 hoặc 128)
  uint16 capacity;            // 2 B: Tổng số slots tối đa (63 hoặc 31)
  uint16 free_count;          // 2 B: Số lượng slots hiện đang trống
  uint16 class_idx;           // 2 B: Chỉ số class sở hữu (0 = 64B, 1 = 128B)
  uint32 _pad;                // 4 B: Đệm căn lề tròn 16 B trước hai con trỏ
  struct slab_slot *freelist; // 8 B: Con trỏ tới slot trống đầu tiên
  struct slab_header *next;   // 8 B: Con trỏ nối sang trang Slab tiếp theo
}; // Tổng kích thước: 32 Bytes

### 3.2. Quy ước Fill vùng nhớ (Memory Poisoning & Zero-fill)

1. Khi tạo trang Slab mới (`slab_NewPage`) và khi giải phóng (`kmfree`):
   * Toàn bộ vùng nhớ slot được lấp đầy bằng mẫu byte `1` (`0x01`, `SLAB_FREE_BYTE`) qua `memset(ptr, 1, c->obj_size)` trước khi ghi con trỏ nhúng `slot->next` vào 8 byte đầu tiên để nối vào đầu `hdr->freelist` (theo cơ chế LIFO).
2. Khi cấp phát đối tượng (`kmalloc`):
   * Sau khi rút `slot` từ đầu `curr->freelist` và nhả khóa `c->lock`, toàn bộ vùng nhớ đối tượng được xóa trắng về `0` (`memset(slot, 0, obj_size)`) trước khi trả về cho người gọi.

### 3.3. Kiểm tra an toàn trong `kmfree(void *ptr)`

1. `ptr == 0`: thoát hàm êm (`no-op`).
2. Địa chỉ ngoài miền vật lý `[end, PHYSTOP)`: gọi `panic("kmfree: pointer out of physical memory range")`.
3. Sai mã nhận diện `hdr->magic != SLAB_MAGIC` hoặc sai metadata class: gọi `panic("kmfree: invalid slab magic")`.
4. Con trỏ nằm trong vùng header (`< hdr + 32`), lệch biên (`offset % obj_size != 0`), hoặc rơi vào vùng dư cuối trang (`slot_idx >= capacity`, chặn 32 B dư của `class64` và 96 B dư của `class128`): gọi `panic`.
5. Trang không thuộc danh sách `c->slabs` của class hoặc phát hiện giải phóng kép (`double free` khi `free_count >= capacity` hoặc `ptr` đã có trong `hdr->freelist`): nhả khóa và gọi `panic`.

### 4. Thống Kê Theo Từng Class (`S04a` — `#38`)

Hàm nội bộ `static int slab_get_class_stats(int idx, struct slab_class_stats *out)` chỉ hợp lệ sau khi `kmalloc_init()` đã chạy (trả về `0` khi thành công, `-1` nếu chưa khởi tạo, `idx` ngoài miền `0..1`, hoặc `out == 0`). Dữ liệu được tính trực tiếp từ danh sách `c->slabs` dưới khóa `c->lock` của từng class và nhả khóa trước khi in:

| Trường trong `slab_class_stats` | Đơn vị | Công thức tính | Ý nghĩa |
| :--- | :--- | :--- | :--- |
| `class_size` | `bytes` | `c->obj_size` (`64` hoặc `128`) | Bước nhảy mỗi slot (stride). |
| `cap_per_slab` | `slots/slab` | `c->capacity` (`63` hoặc `31`) | Số slot tối đa trên mỗi trang Slab. |
| `slab_pages` | `pages` | Số trang trong `c->slabs` | Tổng số trang 4KB cache đang giữ. |
| `empty_slabs` | `pages` | Số trang có `free_count == capacity` | Số trang Slab hoàn toàn rỗng đang giữ lại trong cache. |
| `total_capacity` | `slots` | `slab_pages * cap_per_slab` | Tổng số slot của class hiện có. |
| `live` | `objects` | `total_capacity - free_slots` | Số đối tượng đang được cấp phát (`live + free_slots == total_capacity`). |
| `free_slots` | `slots` | $\sum \text{hdr} \rightarrow \text{free\_count}$ | Tổng số slot đang rảnh trong cache. |
| `page_bytes` | `bytes` | `slab_pages * 4096` | Tổng số byte trang vật lý cache đang giữ. |
| `obj_capacity_bytes` | `bytes` | `total_capacity * class_size` | Tổng số byte dung lượng vùng chứa slot. |
| `live_obj_bytes` | `bytes` | `live * class_size` | Tổng số byte theo stride của các object đang sống. |
| `metadata_bytes` | `bytes` | `slab_pages * 32` | Tổng số byte `slab_header` (chỉ tính header 32 B/trang). |

### 5. Hướng Dẫn Chạy Các Chế Độ Kiểm Thử (`slab_test`)

1. **Chế độ mặc định (`SLAB_TEST_CASE=0`, `SLAB_STATS_TEST=0`):**
   * Chạy tự động self-test `S03a` khi boot (kiểm tra biên `0, 1, 64, 65, 128, 129`, zero-fill, mẫu byte `1` khi free, LIFO reuse, multi-slab, và OOM/retry):
   ```bash
   make clean && make qemu CPUS=1
   ```

2. **Chế độ kiểm thử 3 ca lỗi bắt buộc dừng máy (`SLAB_TEST_CASE=1/2/3`):**
   * `SLAB_TEST_CASE=1` (Double free):
   ```bash
   make clean && make qemu CPUS=1 CC="riscv64-unknown-elf-gcc -DSLAB_TEST_CASE=1"
   ```
   * `SLAB_TEST_CASE=2` (Con trỏ lệch biên slot):
   ```bash
   make clean && make qemu CPUS=1 CC="riscv64-unknown-elf-gcc -DSLAB_TEST_CASE=2"
   ```
   * `SLAB_TEST_CASE=3` (Trang không phải Slab / sai magic):
   ```bash
   make clean && make qemu CPUS=1 CC="riscv64-unknown-elf-gcc -DSLAB_TEST_CASE=3"
   ```

3. **Chế độ kiểm thử thống kê `S04a` (`SLAB_STATS_TEST=1`):**
   * Bật cờ `SLAB_STATS_TEST=1` để đối chiếu và in `expected/actual` tại mốc `init`, các mốc vượt slot trống (`before` -> `live` -> `after_free`) và lượt lặp lại vừa khít `free_slots` sau warm-up (`reuse_live` -> `reuse_free`):
   ```bash
   make clean && make qemu CPUS=1 CC="riscv64-unknown-elf-gcc -DSLAB_STATS_TEST=1"
   ```