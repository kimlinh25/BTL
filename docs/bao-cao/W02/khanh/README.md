# Nguyễn An Khánh - Đóng góp W02

- MSSV: 20224019.
- Tài khoản GitHub: `ankhanh2594`.
- Issue: #19 (N03). PR: #21. Khoảng ngày làm việc thực tế: 24/10/2026, 30–31/10/2026.
- Trạng thái: Partial — xong baseline, test matrix, harness trang (12 ca) đã sửa theo 2 vòng review; ca object (T03) là bước tiếp theo.

## Việc đã làm và kết quả

| Task | Đã làm gì, kết quả cụ thể | Ngày | Đầu ra |
|---|---|---|---|
| E01 | Build/boot baseline tại `578b89f`, `CPUS=3`: boot tới `$`, `free pages after kinit: 32735`, `forktest` → `fork test OK` | 24/10/2026 | `2026-10-24-baseline-boot.txt`, `2026-10-24-env.txt` |
| T01 | Test matrix trang (12 ca) và object (6 ca, chưa chạy), quy tắc an toàn, fault injection, log schema, điều kiện dừng | 30–31/10/2026 | [`docs/tests/test-matrix-allocator.md`](../../../tests/test-matrix-allocator.md) |
| T02 | Bản đầu tại `66d5874`, `CPUS=1`: pass=11, fail=0; inject=1: đúng 3 FAIL | 30/10/2026 | `2026-10-30-kt-run.txt`, `2026-10-30-kt-inject.txt`, `2026-10-30-manifest.txt` |
| T02 | Review lần 1: chặn ca không an toàn, chỉ trả trang sở hữu, P07 kiểm địa chỉ trước khi ghi, harness chỉ link khi bật cờ. Tại `8d9d2bf`: pass=11 fail=0; inject=1 đúng 3 FAIL; inject=2 đúng 7 FAIL | 31/10/2026 | `2026-10-31-kt-run.txt`, `2026-10-31-kt-inject1.txt`, `2026-10-31-kt-inject2.txt`, `2026-10-31-manifest.txt` |
| T02 | Review lần 2: ghép `main` sau #22/#23, đặt harness ngay sau `kinit()` trước `slab_test()`; P07 phát hiện trang trùng bằng bitmap ngoài trang, thêm P07e và `INJECT=3`. Tại `671e120`, `CPUS=1`: default không có harness, `slab_test` đạt; pass=12 fail=0; inject=1 đúng 3 FAIL; inject=2 đúng 7 FAIL; inject=3 đúng 3 FAIL; mọi lần boot tới shell | 31/10/2026 | `2026-10-31-v2-default.txt`, `2026-10-31-v2-kt-run.txt`, `2026-10-31-v2-kt-inject1.txt`, `2026-10-31-v2-kt-inject2.txt`, `2026-10-31-v2-kt-inject3.txt`, `2026-10-31-v2-manifest.txt` |
| T02 | Review lần 3: P07c chỉ `kfree` trang chưa có trong bitmap (tránh trả 2 lần khi `kalloc` trả trang cũ thay vì 0), thêm `INJECT=4`. Tại `0788d4b`, `CPUS=1`: default không có harness, `slab_test` đạt; pass=12 fail=0; inject=1/2/3/4 lần lượt đúng 3/7/3/1 FAIL; mọi lần boot tới shell | 31/10/2026 | `2026-10-31-v3-default.txt`, `2026-10-31-v3-kt-run.txt`, `2026-10-31-v3-kt-inject1.txt`, `2026-10-31-v3-kt-inject2.txt`, `2026-10-31-v3-kt-inject3.txt`, `2026-10-31-v3-kt-inject4.txt`, `2026-10-31-v3-manifest.txt` |

Ghi nhận: số trang trống lúc boot đổi theo kích thước kernel (thêm harness, thêm bitmap), nên harness luôn so với số đếm lúc bắt đầu, không báo nhầm thành leak.

## Khó khăn và TODO kỳ sau

- Khó khăn:
  - Hiện chưa có.
- TODO W03: T03 (harness object trên `kmalloc`/`kmfree`); chạy lại harness trang trên bản per-CPU với nhiều CPU, cần nhóm xác nhận lại trước.
