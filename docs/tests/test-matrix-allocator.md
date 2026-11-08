# Test matrix allocator (T01), harness trang (T02) và harness object (T03) — Khánh

Issue: #19 (N03). Bản đầu 30/10/2026. Cập nhật 31/10/2026 theo review PR #21 lần 1 (chặn ca không an toàn, kiểm tra địa chỉ trong P07, sửa mô tả cờ build) lần 2 (chống trang trùng trong P07, ghép `main` sau #22/#23, đổi tên API object) và lần 3 (P07c không trả lại trang đã có trong bitmap). Cập nhật 06/11/2026 (W03): thêm harness object T03 (mục 5, 6, 7, 8). Cập nhật 08/11/2026 theo review PR #33: O04b bắt cấp lại NULL/sai/trùng, dọn stash không free trùng (C01), `st_valid` chống tràn (V01), thêm `SLABTEST_INJECT=3/4`.

## 1. Cách kích hoạt

Harness tầng trang nằm ở `kernel/kalloctest.c`. Chỉ khi bật cờ `KALLOCTEST`, `Makefile` mới biên dịch và link `kalloctest.o`, và `main.c` mới gọi `kalloctest()`. Build mặc định không chứa harness.

```bash
make clean
make qemu CPUS=1                                    # mặc định: không có harness
make clean
make qemu CPUS=1 KALLOCTEST=1                       # chạy thật
make clean
make qemu CPUS=1 KALLOCTEST=1 KALLOCTEST_INJECT=1   # tự kiểm tra: các ca bắt được lỗi
make clean
make qemu CPUS=1 KALLOCTEST=1 KALLOCTEST_INJECT=2   # tự kiểm tra: đường chặn NULL/địa chỉ sai/trùng
make clean
make qemu CPUS=1 KALLOCTEST=1 KALLOCTEST_INJECT=3   # tự kiểm tra: trang trùng trong P07
make clean
make qemu CPUS=1 KALLOCTEST=1 KALLOCTEST_INJECT=4   # tự kiểm tra: P07c nhận trang đã có trong bitmap
```

Đổi cờ thì phải `make clean`, vì object cũ không tự biên dịch lại khi chỉ đổi cờ.

- Harness chạy trên CPU 0 ngay sau `kinit()`, trước `kmalloc_init()`/`slab_test()` và trước khi CPU khác khởi động. Đây là trạng thái yên: chưa ai khác lấy trang, nên số trang đếm được là chính xác.
- Mọi so sánh số trang dùng `before = kfreepage_count()` đếm lúc bắt đầu, không viết cứng số trang lúc boot, vì số này đổi theo kích thước kernel.
- W02 chỉ chạy `CPUS=1`. Nhiều CPU và stealing là W03 (P05 trong backlog).

## 2. Quy tắc an toàn của harness

- Địa chỉ hợp lệ (`kt_valid`): khác 0, chia hết `PGSIZE`, nằm trong `[PGROUNDUP(end), PHYSTOP)`.
- Trang được test "sở hữu" khi hợp lệ (P01) và không trùng trang khác (P03). Trang trùng chỉ giữ bản đầu tiên.
- Nếu có trang sai hoặc trùng (`kt_unsafe`), ca ghi vào trang (P04) bị bỏ qua và tính FAIL (`note=skipped_unsafe`).
- Khi dọn dẹp chỉ `kfree` trang đang sở hữu, không bao giờ `kfree` địa chỉ sai hoặc trả một trang hai lần.
- P07 không ghi gì vào các trang đã cấp. Trang đã cấp được đánh dấu bằng bitmap 1 bit/trang (4 KB) nằm ngoài trang. Mỗi trang phải hợp lệ và chưa có bit; gặp trang sai hoặc trùng thì dừng vòng lặp và bỏ qua P07c. P07c chỉ `kfree` trang chưa có trong bitmap. Dọn dẹp duyệt bitmap, mỗi trang trả đúng một lần.

## 3. Ca kiểm thử tầng trang (`kalloc`/`kfree`)

N = 64 trang sống cùng lúc. Cột cuối là kết quả chế độ thường (`INJECT=0`).

| ID | Kiểm tra | Cách làm | Expected | Điều kiện FAIL | Kết quả |
|---|---|---|---|---|---|
| P01 | Căn lề và phạm vi | Cấp N trang, kiểm `kt_valid` | Cả N trang hợp lệ | Có trang NULL/lệch/ngoài phạm vi | PASS |
| P02 | Junk fill khi cấp | Đọc 4096 byte mỗi trang sở hữu | Mọi byte = 5 | Có byte ≠ 5, hoặc thiếu trang sở hữu | PASS |
| P03 | Không cấp trùng | So từng cặp trang sở hữu | 0 cặp trùng | Có cặp trùng | PASS |
| P04 | Ghi/đọc pattern | Ghi `(i << 32) \| j` vào mọi ô 8 byte, ghi hết rồi mới đọc | N trang nguyên vẹn | Có trang bị đè, hoặc bị bỏ qua vì không an toàn | PASS |
| P05a | Đếm sau cấp | `kfreepage_count()` | `before − N` | Khác | PASS |
| P05b | Bảo toàn sau trả (leak) | Trả trang sở hữu rồi đếm | `before` | Khác (thiếu = leak) | PASS |
| P06 | Junk fill khi trả | Cấp 1, trả, đọc byte 8..4095 | Đều = 1 (8 byte đầu là con trỏ `next`) | Có byte ≠ 1, hoặc trang cấp ra không hợp lệ | PASS |
| P07a | Cấp đến hết (OOM) | `kalloc` đến khi trả 0 | Số trang cấp được = `before` | Khác, hoặc dừng sớm vì trang sai/trùng | PASS |
| P07b | Địa chỉ khi OOM | `kt_valid` từng trang | 0 trang sai | > 0 | PASS |
| P07e | Không trùng khi OOM | Kiểm bit của trang trong bitmap trước khi đánh dấu | 0 trang trùng | > 0 | PASS |
| P07c | Hết bộ nhớ | `kalloc` thêm 1 lần; nếu nhận trang thì chỉ trả khi trang chưa có trong bitmap | Trả 0 | ≠ 0, hoặc bị bỏ qua vì P07b/P07e | PASS |
| P07d | Bảo toàn sau OOM | Trả mỗi trang có bit đúng một lần rồi đếm | `before` | Khác | PASS |

P02 và P06 kiểm tra hành vi junk 5/1 của `kalloc.c`. Nếu bản per-CPU đổi giá trị junk thì cập nhật expected, không xoá ca.

## 4. Tự kiểm tra harness (fault injection)

Lỗi giả lập chỉ tác động lên dữ liệu của harness, trang thật luôn được trả lại, nên kernel vẫn boot bình thường và `free_after` bằng `free_before`.

| Chế độ | Giả lập | Expected |
|---|---|---|
| `INJECT=1` | P03: tăng bộ đếm trùng; P04: đè 1 ô trong trang 3; P05b: tạm giữ 1 trang | 3 FAIL: P03, P04, P05b |
| `INJECT=2` | P01: trang 2 thành NULL, trang 4 trùng trang 3, trang 6 lệch 8 byte; P07: trang thứ 11 lệch 8 byte | 7 FAIL: P01, P02, P03, P04 (bỏ qua), P07a, P07b, P07c (bỏ qua). Kernel boot tới shell |
| `INJECT=3` | P07: lần cấp thứ 11 trả lại trang vừa cấp | 3 FAIL: P07a (dừng ở 10), P07e (1), P07c (bỏ qua). P07d PASS, kernel boot tới shell |
| `INJECT=4` | P07c: `kalloc` trả lại trang đã có trong bitmap thay vì 0 | 1 FAIL: P07c. P07d PASS (mỗi trang trả đúng một lần), kernel boot tới shell |

## 5. Ca kiểm thử tầng object (slab, T03)

Harness tầng object nằm ở `kernel/slabtest.c`. Chỉ khi bật cờ `SLABTEST`, `Makefile` mới biên dịch và link `slabtest.o`, và `main.c` mới gọi `slabtest()`. Cờ này độc lập với `KALLOCTEST`. Build mặc định không chứa harness.

```bash
make clean
make qemu CPUS=1 SLABTEST=1                     # chạy thật
make clean
make qemu CPUS=1 SLABTEST=1 SLABTEST_INJECT=1   # tự kiểm tra: các ca bắt được lỗi
make clean
make qemu CPUS=1 SLABTEST=1 SLABTEST_INJECT=2   # tự kiểm tra: đường chặn NULL/lệch/trùng
make clean
make qemu CPUS=1 SLABTEST=1 SLABTEST_INJECT=3   # tự kiểm tra: O04b bắt cấp lại sai
make clean
make qemu CPUS=1 SLABTEST=1 SLABTEST_INJECT=4   # tự kiểm tra: dọn stash không free trùng
```

Hợp đồng đang test (`contract=slab64`, `main` sau #23): một class 64 B, header 32 B, 63 slot/trang; `kmalloc(size)` nhận `1..64`, `size = 0` hoặc `> 64` trả 0; `kmfree(0)` là no-op.

- Harness chạy trên CPU 0 ngay sau `slab_test()`, một lần, không gọi lại `kmalloc_init()`. Lúc này cache đã giữ sẵn trang slab rỗng do `slab_test()` tạo, nên harness không giả định cache rỗng và mọi so sánh dùng số đếm lúc bắt đầu.
- Object hợp lệ (`st_valid`): khác 0, căn lề 8 B, `[p, p+size)` nằm trong `[PGROUNDUP(end), PHYSTOP)`, trang chứa có magic `SLAB`, đúng biên slot `32 + k * obj_size` với `k < capacity`, và `size <= obj_size`. Kiểm tra phạm vi dạng `p < PHYSTOP` và `size <= PHYSTOP − p` để `p + size` không tràn. Chỉ đọc header sau khi biết địa chỉ nằm trong RAM.
- Ownership như mục 2: object được sở hữu khi hợp lệ (O01) và không chồng lấn object khác hoặc object trong stash (O02). Có object sai/trùng thì O03 bị bỏ qua và tính FAIL. Object cấp lại (O04) cũng chỉ được sở hữu khi hợp lệ và không chồng lấn.
- Stash: khi inject thay địa chỉ thật bằng địa chỉ giả, object thật được cất vào stash. Dọn dẹp trả stash trước, nhưng bỏ qua object trùng object đang sở hữu hoặc lặp lại trong stash (đếm ở C01), rồi mới `kmfree` object đang sở hữu. Mỗi object được trả đúng một lần.
- Trang slab rỗng được cache giữ lại, không trả về `kalloc`. Vì vậy không yêu cầu số trang trống quay về mức trước lần cấp đầu, và trang cache giữ không bị tính là leak. O06 chỉ kiểm tra số trang ổn định sau warm-up.

N = 200 object sống cùng lúc (hơn 3 trang), size yêu cầu lặp theo `1, 8, 13, 32, 63, 64`. Cột cuối là kết quả chế độ thường tại `9c5a5df`, `CPUS=1`.

| ID | Kiểm tra | Cách làm | Expected | Điều kiện FAIL | Kết quả |
|---|---|---|---|---|---|
| B01 | Kích thước biên | `kmalloc` size 0, 1, 64, 65, 128, 129; object cấp được thì trả ngay | 1 và 64 hợp lệ; 0, 65, 128, 129 trả 0 | Khác hợp đồng | PASS (6 ca) |
| B02 | `kmfree(0)` | Gọi `kmfree(0)` | Không panic | Kernel panic | PASS |
| O01 | Hợp lệ, căn lề | Cấp N object, kiểm `st_valid` | Cả N hợp lệ | Có object NULL/lệch/sai slot | PASS |
| V01 | `st_valid` từ chối con trỏ xấu | Gọi `st_valid` với 8 con trỏ xấu: NULL, lệch 4, lệch 8 trong slot, trong header, `KERNBASE`, `PHYSTOP`, `PHYSTOP − 8` size 64, `0xFFFFFFFFFFFFFFF8` size 64 | Từ chối cả 8, object 0 vẫn hợp lệ | Nhận con trỏ xấu, hoặc kernel lỗi khi đọc header | PASS |
| O02 | Không trùng, không chồng lấn | So từng cặp vùng `[p, p+size)` của object sở hữu | 0 cặp | Có cặp chồng lấn | PASS |
| O03 | Ghi/đọc pattern | Ghi mẫu riêng theo đúng size yêu cầu, ghi hết rồi đọc lại, kiểm lại header | N object nguyên vẹn | Có object bị đè, header hỏng, hoặc bị bỏ qua vì không an toàn | PASS |
| O04a | Reuse cùng slot | Trả object cấp sau cùng rồi cấp lại cùng size | Cùng địa chỉ (LIFO) | Khác địa chỉ | PASS |
| O04b | Reuse hợp lệ, không tốn trang | Trả 10 object rồi cấp lại 10 | 10/10 object cấp lại hợp lệ, không chồng lấn; `kfreepage_count()` không đổi | Có object cấp lại NULL/sai/trùng, hoặc số trang đổi | PASS |
| O05a | Vượt 1 slab | Đếm trang chứa các object sở hữu | ≥ `ceil(n/63)` trang, không trang nào quá `capacity` | Ít hơn, hoặc trang quá `capacity` | PASS (4 trang) |
| O05b | Trang mới khi vượt cache | `before − mid` sau khi cấp N | ≤ `ceil(N/63)` = 4 | Lớn hơn | PASS (lấy 1 trang) |
| C01 | Dọn dẹp không free trùng | Đếm object trong stash bị bỏ qua vì trùng object đang sở hữu hoặc lặp lại | 0 | > 0 | PASS |
| O06 | Ổn định sau warm-up | 1 chu kỳ warm-up, rồi 5 chu kỳ cấp/trả N object | 0 chu kỳ làm đổi số trang trống | > 0, hoặc có object không sở hữu được | PASS |

Kết quả chế độ thường: `pass=17|fail=0`, `free_before=32729`, `free_after=32728`. Chênh 1 trang là trang slab mới do O05 tạo và được cache giữ lại, không phải leak (`o06_base = o06_low = 32728`).

Tự kiểm tra harness object:

| Chế độ | Giả lập | Expected |
|---|---|---|
| `SLABTEST_INJECT=1` | O02: tăng bộ đếm chồng lấn; O03: đảo 1 byte trong vùng của object 3; O06: tạm giữ 1 trang ở chu kỳ 3 | 3 FAIL: O02, O03, O06. Kernel boot tới shell |
| `SLABTEST_INJECT=2` | O01: object 2 thành NULL, object 4 trùng object 3, object 6 lệch 8 byte; object thật được cất vào stash và trả khi dọn | 3 FAIL: O01 (198), O02 (1), O03 (bỏ qua). `free_after` bằng chế độ thường, kernel boot tới shell |
| `SLABTEST_INJECT=3` | O04b: lần cấp lại object 10 trả NULL, object 11 trả địa chỉ lệch 8 byte, object 12 trả object 30 đang sống; object thật được cất vào stash | 1 FAIL: O04b (7/10). Kernel boot tới shell |
| `SLABTEST_INJECT=4` | Trước khi dọn: đưa object 5 đang sở hữu vào stash 2 lần | 1 FAIL: C01 (2). Object 5 chỉ được trả một lần, kernel boot tới shell |

Chưa chạy hoặc phụ thuộc:

- Class128 (N02) chưa vào `main`. Khi vào: đổi bảng `st_bounds` (65 và 128 cấp được, 129 vẫn trả 0), đổi `ST_CONTRACT`, thêm ca object 64 B và 128 B sống cùng lúc, và ghi revision đổi hợp đồng.
- OOM/retry tầng object dùng lại test có kiểm soát trong `slab_test()` (S02/N02). Không cấp hết RAM. OOM sâu cho T03 chuyển sang W04.
- Chỉ chạy `CPUS=1`. Harness đồng thời 1/2/4/8 CPU không thuộc W03.

## 6. Log schema

Mỗi dòng của harness bắt đầu bằng `KT|`, lọc bằng `grep -a '^KT|'`.

```text
KT|begin|rev=<git describe>|cpus=<CPUS>|inject=<0/1/2/3/4>|n=<N>|free_before=<số trang>
KT|case=<ID>|expected=<mô tả>|want=<giá trị mong đợi>|actual=<giá trị thật>|result=<PASS/FAIL>
KT|case=<ID>|expected=safe_input|result=FAIL|note=skipped_unsafe
KT|end|pass=<số>|fail=<số>|free_after=<số trang>|summary=<ALL_PASS/HAS_FAIL>
```

Harness object (T03) dùng cùng dạng dòng `case`, thêm trường `harness=object` ở `begin`/`end` để tách khỏi harness trang:

```text
KT|begin|harness=object|rev=<git describe>|cpus=<CPUS>|inject=<0/1/2/3/4>|contract=<slab64>|n=<N>|free_before=<số trang>
KT|info|o04b_free_before=<số trang>|o04b_free_after=<số trang>
KT|info|o06_base=<số trang>|o06_low=<số trang>|cycles=<số chu kỳ>|bad=<số object lỗi>
KT|end|harness=object|pass=<số>|fail=<số>|free_after=<số trang>|summary=<ALL_PASS/HAS_FAIL>
```

Manifest đi kèm mỗi lần chạy: người chạy, ngày, revision, OS, toolchain, QEMU, lệnh, `CPUS`. `rev` có đuôi `-dirty` nghĩa là chạy trên code chưa commit, không dùng làm kết quả nộp.

## 7. Điều kiện dừng

- PASS toàn bộ: dòng `KT|end` có `pass=12|fail=0`, và kernel boot tiếp tới `init: starting sh`.
- FAIL: có dòng `result=FAIL`, hoặc kernel `panic`, hoặc không xuất hiện `KT|end` sau 60 giây.
- `INJECT=1` phải ra đúng 3 FAIL; `INJECT=2` đúng 7 FAIL; `INJECT=3` đúng 3 FAIL; `INJECT=4` đúng 1 FAIL; tất cả vẫn boot tới shell. Khác đi nghĩa là harness hỏng.
- Harness object: PASS toàn bộ khi `KT|end|harness=object` có `pass=17|fail=0` và kernel boot tới `init: starting sh`. `SLABTEST_INJECT=1` và `SLABTEST_INJECT=2` mỗi chế độ đúng 3 FAIL; `SLABTEST_INJECT=3` và `SLABTEST_INJECT=4` mỗi chế độ đúng 1 FAIL; tất cả vẫn boot tới shell. Điều kiện FAIL và thời hạn 60 giây giống harness trang.

## 8. Yêu cầu ghép file dùng chung

Nhánh này thêm vài dòng vào file dùng chung, đều nằm sau cờ `KALLOCTEST`:

- `Makefile`: khối `ifdef KALLOCTEST` thêm `$K/kalloctest.o` vào `OBJS` và các cờ `-DKALLOCTEST`, `-DKALLOCTEST_INJECT`, `-DKT_CPUS`, `-DKT_REV`. Build mặc định không biên dịch, không link harness.
- `kernel/defs.h`: khai báo `void kalloctest(int);`.
- `kernel/main.c`: gọi `kalloctest(KALLOCTEST_INJECT)` ngay sau `kinit()`, trước `kmalloc_init()`/`slab_test()`, trong `#ifdef KALLOCTEST`.

T03 (W03) thêm các dòng sau, đều nằm sau cờ `SLABTEST`:

- `Makefile`: khối `ifdef SLABTEST` thêm `$K/slabtest.o` vào `OBJS` và các cờ `-DSLABTEST`, `-DSLABTEST_INJECT`, `-DST_CPUS`, `-DST_REV`. Build mặc định không biên dịch, không link harness.
- `kernel/defs.h`: khai báo `void slabtest(int);`.
- `kernel/main.c`: gọi `slabtest(SLABTEST_INJECT)` một lần ngay sau `slab_test()`, trong `#ifdef SLABTEST`. Không gọi lại `kmalloc_init()`.
