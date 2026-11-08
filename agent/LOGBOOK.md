# Nhật ký dự án

Cập nhật: 31/10/2026. Lưu kết quả có ý nghĩa; transcript nằm riêng trong `history/`.

## L033 - 31/10/2026 - Hoàn thiện và kiểm chứng Slab W02

- request: Linh yêu cầu làm theo `docs/w02_check.md`, sau đó cập nhật file checklist và nhờ kiểm tra lại.
- branch sync: Đã fetch `origin/main` tại `13c39f3` và phát hiện xung đột ở CURRENT_STATE/RECOVERY_NOTES. WIP được giữ trong `stash@{0}` trước khi merge; nội dung main về W02/P02 được giữ, phần slab được nối vào. Commit merge `3a9612c` do Minhlth tạo khi phiên trước bị ngắt; lượt này chỉ làm việc trên WIP sau merge, không commit/push. Snapshot pre-merge vẫn còn ở stash.
- code: `main()` gọi `kmalloc_init()` và kế tiếp `slab_test()` sau `kinit()`. Test helper không tự init lại cache. `slab_test()` kiểm tra boundary/NULL, căn lề 8 byte, LIFO reuse, pattern, cấp phát slab thứ hai và fault-injected OOM/retry. Makefile có `SLAB_TEST_CASE=1/2/3` để chạy riêng double-free, misaligned pointer và non-slab pointer panic tests.
- documentation: Viết lại `docs/slab_allocator_design.md` để khớp embedded freelist, header 32B, stride 64B, capacity 63, alignment 8B, API/failure paths và kết quả kiểm thử. Checklist `docs/w02_check.md` hiện untracked và kết thúc ngay sau mục `make fmt`/`git diff --check`; chưa tự sửa file checklist.
- verification: `make fmt`, `git diff --check`, `clang-format --dry-run --Werror kernel/main.c kernel/slab.c` đạt. `make clean && make TOOLPREFIX=riscv64-linux-gnu- kernel/kernel fs.img` đạt. QEMU normal boot in `slab_test: ALL PASSED (capacity=63, obj_size=64)` và tiến tới shell. `./test-xv6.py usertests` in `ALL TESTS PASSED`; `./test-xv6.py crash` qua log recovery và hai orphan recovery. Các build panic riêng lần lượt in expected: `panic: kmfree: double free detected`, `panic: kmfree: unaligned slot pointer`, `panic: kmfree: invalid slab magic or corrupted header`.
- status: Không có conflict marker/unmerged file sau merge. Branch local `codex/slab-allocator_page` có hai commit ahead upstream tại lần đọc cuối; source/docs WIP còn modified và `docs/w02_check.md` untracked. Không stage/commit/push thay Linh. Slab chưa được tích hợp vào client kernel; test hiện là boot test.
- limits: Checklist hiện tại chỉ liệt kê format/whitespace trong phần mục 4 rồi kết thúc; chưa có yêu cầu log/CI tiếp theo trong nội dung file đang đọc. CI `git diff --exit-code` không được tuyên bố pass vì WIP có thay đổi mong muốn.

## L032 - 31/10/2026 - Sửa lỗi build slab và kiểm tra CI local

- request: Linh yêu cầu chạy check/test và hỗ trợ sửa lỗi từ ảnh CI đang dừng ở `make fmt && git diff --exit-code`; sau đó yêu cầu chạy kiểm tra lại.
- context: Nhánh local `codex/slab-allocator_page`, HEAD `8241d43`; trước khi sửa, working tree đã có thay đổi định dạng trong `kernel/slab.c`. Giữ các thay đổi đó, không stage/commit/push.
- diagnosis: Sau khi build với `TOOLPREFIX=riscv64-linux-gnu-`, trình biên dịch báo `printf` implicit declaration trong `slab_test()`. Kernel cung cấp `printk()` qua `defs.h`, không phải `printf()`.
- repair: Đổi hai thông báo trong `slab_test()` sang `printk()`. Không gọi `slab_test()` trong `main()` vì lựa chọn boot/integration chưa được yêu cầu; hiện helper chưa được gọi và các bài `usertests`/`crash` không kiểm tra allocator này.
- verification: Lần đầu `make -o /usr/lib/gcc/riscv64-unknown-elf/13.2.0/include/stdarg.h TOOLPREFIX=riscv64-linux-gnu- -B kernel/kernel fs.img` hoàn tất với exit 0. Lần chạy lại, `make fmt` chạy hai lần; `clang-format --dry-run --Werror kernel/slab.c` và `git diff --check` đều đạt. `make kernel/kernel fs.img` báo cả hai target up to date (binary đã build thành công ở lượt đầu). `./test-xv6.py usertests` in `ALL TESTS PASSED`; `./test-xv6.py crash` qua log recovery, orphan recovery và in `OK` cho cả hai test orphan. Sau kiểm tra đã dừng QEMU process còn sót do script và file FUSE tạm biến mất; không còn artifact đó trong status.
- limits: `make fmt && git diff --exit-code` nguyên dạng không được xác nhận pass vì worktree có thay đổi code/docs có chủ đích. Formatter ổn định được kiểm tra bằng lần chạy lặp, dry-run file slab và whitespace check. CI usertests/crash không gọi `slab_test()`; chưa có runtime test trực tiếp cho slab. Lệnh build mặc định local trước đó chọn toolchain `unknown-elf` và bị dependency `.d` cũ trỏ tới header không tồn tại; build thành công khi ép toolchain Linux và bỏ qua prerequisite đó.

## L031 - 23/10/2026 - Gỡ xung đột local và giữ đúng hai ảnh minh chứng

- authorization: Linh yêu cầu assistant tự xử lý và commit, xóa hai file linh-chung; sau đó yêu cầu rõ tự commit và tự push. Quyền stage/commit/push chỉ áp dụng lần sửa này, chưa yêu cầu merge thay người dùng.
- publication: Đã push thành công nhánh codex/fix-w01-evidence, thiết lập upstream và đối chiếu SHA HEAD với remote. Lúc kiểm tra mới, nội dung đã được commit và working tree sạch nên không cần tạo lại commit cho ảnh. Hai commit fix/revert của Linh sau 8b49680 triệt tiêu thay đổi của nhau; giữ nguyên lịch sử, không force-push. Nhánh sửa có đúng 2 ảnh minh-chung và README, chưa đưa phần sửa vào main hoặc tạo PR trong lượt này.
- diagnosis: Local main b00fe36 lệch với origin/main; b00fe36 xóa docs/onboarding/W1_Minh.md và minh.md, không phải ảnh. Stash 65a2e92 được áp khi quay về main gây UU ở LOGBOOK, RECOVERY_NOTES và README W01/linh. Origin/main5c5fb99 (PR #9) có bốn tên ảnh nhưng mỗi cặp linh/minh có blob giống hệt nhau.
- backup: Đã sao lưu working copies, 9 stage blobs, raw index, refs/reflog và full Git bundle --all --reflog, kiểm tra đọc lại/CRC và hash, tại C:/Users/KimSakota/.codex/backups/BTL-conflicts-20261023-aa7t56qa/. Gói repository-before-repair.bundle giữ được commit b00fe36 và các stash cũ trước khi sửa.
- repair: Sau xác nhận không có file ngoài dự kiến và implementation trees không khác, đồng bộ main local về origin/main5c5fb99; giữ hai ghi chú Minh. Tạo một nhánh codex/fix-w01-evidence từ main mới, lấy lại nội dung mới nhất của 3 Markdown từ stage 3 đã backup, giữ README ngắn 3 phần và dẫn 01-minh-chung.png/02-minh-chung.png. Chỉ xóa 01-linh-chung.png và 02-linh-chung.png. Không sửa ảnh giữ lại hoặc source C/h/S.
- stash handling: Nội dung và các thay đổi xóa trong stash65a2e92 đã được đối chiếu với yêu cầu: giữ sửa 3 Markdown, thực hiện xóa 2 ảnh linh-chung, bỏ ý định xóa nhầm ghi chú Minh. Stash này đã lưu bundle và được bỏ khỏi danh sách để tránh áp lại xung đột. Stash5221c62 chứa hướng dẫn thành viên từ trước vẫn giữ nguyên.
- memory: Cập nhật checkpoint/CURRENT_STATE đúng main5c5fb99, PR #8/#9 đã merge, canonical ảnh là minh-chung theo lựa chọn cuối của Linh. Không tạo thêm thay đổi bộ nhớ chỉ để ghi từng lần bấm nút.
- scope and validation: Chỉ thay Markdown/bộ nhớ và xóa hai ảnh trùng. Đối chiếu index không còn unmerged, kiểm tra link/ảnh của README và diff whitespace; implementation, workflow và nội dung hai ảnh giữ lại phải khớp base. Không chạy build/boot/test vì phần sửa này không thay implementation.

## L030 - 23/10/2026 - README đóng góp và hai ảnh W01 của Linh

- request: Linh đã thêm hai ảnh vào thư mục cá nhân, muốn được hướng dẫn viết README và làm bước tiếp theo.
- verified: Nhánh hiện tại codex/linh-onboarding ở ad79434, cùng origin; PR #8 Create onboarding còn mở lúc đọc web. Commit này đã có docs/onboarding/linh.md (5 dòng) và 01-minh-chung.png; 02-minh-chung.png còn untracked trước lượt sửa. Không hướng dẫn tạo lại nhánh nop_bao_cao hoặc mở PR khác.
- images reviewed: Ảnh 1 là GitHub Desktop tại main94f56a2/PR #6, xem History và diff ghi chú Minh; chỉ chứng minh thao tác đồng bộ/xem thay đổi, không phải test do Linh chạy. Ảnh 2 là Files changed của PR #8, có ghi chú onboarding Linh và ảnh 1 tại thời điểm chụp. Hai ảnh chụp 23/10; không dùng trạng thái trong ảnh thay cho trạng thái GitHub mới hơn.
- changes: Điền bản README local W01/linh với thông tin người đóng góp, bảng công việc có link PR #2/#4/#8, phần học trước dẫn ghi chú gốc, caption và link tương đối tới cả hai ảnh. Có phần chốt W01 và TODO kỳ sau ở trạng thái đề xuất. Không gán build/boot/forktest/benchmark của Minh cho Linh hoặc nhận đã triển khai slab/per-CPU.
- follow-up: Linh muốn được hướng dẫn lại từ đầu. README được rút gọn thành 3 phần: việc đã làm, hai ảnh có caption và việc tiếp theo. Link ảnh dùng 01-linh-chung.png/02-linh-chung.png, kiểm tra cả hai tồn tại. Các cặp tên minh-chung và linh-chung có cùng nội dung ở lần đọc đầu; người dùng đang tự dọn tên cũ trong lúc làm, assistant không xóa/stage ảnh.
- next: Lần kiểm tra mới, codex/linh-onboarding ở 322810e đã cùng origin, còn README mới chưa commit. origin/main đã có a4ed7be Create onboarding (#8): PR #8 đã merge, không tiếp tục nói push sẽ cập nhật PR #8 đang mở. Nếu đưa phần bổ sung lên main cần PR mới từ nhánh công việc, sau khi Linh tự commit/push. Local main có commit b00fe36 Xóa 2 file minh, ahead1/behind1 so với origin/main; chưa xử lý lệch lịch sử và không tự reset/force-push. Giữ nguyên index/user changes, chỉ sửa README và checkpoint.
- current context: main tracking ở 94f56a2, đã có PR #4 (fd42c53), PR #5 và PR #6; ghi chú Minh W1_Minh.md có nội dung và username Minhlth. Kết quả môi trường/test trong ghi chú Minh là tự báo cáo, chưa đối chiếu raw log/ảnh của Minh trong lượt này. Nhận định cũ Minh chỉ có file rỗng đã được thay thế.

## L029 - 23/10/2026 - Tổ chức hồ sơ báo cáo và minh chứng theo tuần

- request: Linh muốn lưu ảnh/tài liệu báo cáo trong repo, chia thư mục theo tuần. Linh xác nhận W01 mới tạo cấu trúc dự án và dùng GitHub; Minh/Khánh đã được thêm vào repo, không mời lại.
- changes: Đã tạo local docs/bao-cao/W01/README.md và linh/minh/khanh/chung/README.md. Mỗi người điền đóng góp, lưu ảnh/log và dẫn PR trong thư mục mình; phần chung lưu bằng chứng nhóm và thông tin nộp. Báo cáo tổng hợp .md/.pdf sau này đặt tại W01. W02 trở đi tạo cùng cấu trúc khi bắt đầu kỳ mới.
- integration: Bổ sung hướng dẫn đặt tên ảnh, link tương đối, cách gửi PR bằng Desktop và quy trình nộp trong docs/bao-cao/README.md; nối từ README root và sổ tay. Giữ nháp cũ ở vị trí hiện tại để không gãy link. Không tạo ảnh/log/kết quả giả hoặc bản nộp cuối, không sửa source C/h/S.
- validation: 8 Markdown liên quan và 44 target tương đối hiện có đã kiểm tra, các fence cân bằng; link thành viên-guide bị thiếu từ trước nằm ngoài thay đổi và không được tính là link mới đã sửa. 5 Markdown W01 mới kiểm tra không có trailing whitespace; git diff --check đạt. Không cần build/test cho thay đổi tài liệu này.
- workflow: Chưa stage/commit/push, không tạo branch/worktree. Linh tự tạo nhánh codex/docs-weekly-evidence qua Desktop, chọn phần tài liệu thuộc việc này, commit/push và mở PR vào main. Sau merge, Minh/Khánh pull main rồi mỗi người gửi PR minh chứng riêng theo tuần. Lưu trên repo không thay việc đính kèm báo cáo vào thread môn học.
- context correction: PR #2 đã merge và snapshot VstHelper đã có trên main; kiểm tra trước trong chat thấy main/origin/main c9b0128 với commit summary (#3) thêm file onboarding Minh còn rỗng. Những nhận định PR #2 còn mở/chưa có thành viên trong checkpoint 19/10 là lịch sử, không áp dụng lại.

## L027 - 19/10/2026 - Chuẩn bị VstHelper để chia sẻ trong BTL

- type: user-request / observation / preparation
- request: Linh muốn up thư mục VstHelper và hỏi nên tạo branch hay đưa thẳng main. Hướng dẫn nhánh mới codex/reference-vsthelper rồi PR, giữ quy trình main đã bảo vệ; chốt phần docs hiện tại trong PR #2 trước khi tạo nhánh mới từ main đã cập nhật.
- source: VstHelper là repo Git riêng, origin https://github.com/kimsakota/ShiftSort.git, HEAD7236e72f83c2b780bf37fa42c9cf601d3d6db35c trên master; có modified và untracked thực tế. Không coi remote/HEAD là toàn bộ nội dung working tree.
- prepared: Tạo references/VstHelper gồm 43 file nguồn/tài liệu (1.213.206 byte), thêm README và SNAPSHOT.json ghi remote, commit nền, trạng thái dirty, danh mục và SHA-256 nguồn tại lúc chụp. Bao gồm thay đổi/file mới không bị ignore, loại Git/IDE/build/user settings và file phụ trợ TeX. Đây là snapshot tham khảo C#, không tích hợp vào build xv6; số liệu/tài liệu nguồn không tự là kết quả đo BTL.
- preservation: Repo gốc và Git history giữ nguyên, kiểm tra HEAD/status và hash 43 file đều không đổi. Bổ sung /VstHelper/ vào .gitignore của BTL để giữ clone riêng ở local; bản chia sẻ là references/VstHelper không có nested .git. Checksum snapshot là byte trước Git EOL filters.
- validation: So SHA-256 43 file source/copy, xác nhận không có .git/.vs/bin/obj trong snapshot, original .git còn và status nguyên. Không build/test C# hoặc xv6; chưa stage/commit/push/merge.
- remaining: Cần Linh tự chốt 19 Markdown workflow đang sửa trên PR #2, rồi main fetch/pull và nhánh riêng chỉ commit .gitignore + references/VstHelper. Mời Minh/Khánh và board vẫn là phần workflow còn tiếp; không quay lại giao C.

## L026 - 19/10/2026 - Hoàn thiện hướng dẫn cộng tác ba người theo yêu cầu Linh

- type: user-correction / research / documentation
- request: Linh đang cần hướng dẫn GitHub cho cả nhóm, chưa muốn quay lại Lesson 3. Cần thêm Minh/Khánh, đăng lesson để mọi người đọc, lộ trình và cách nhận/làm/review việc. Assistant đã chuyển sang bài C quá sớm; sửa ưu tiên theo yêu cầu này.
- live evidence: Repo public, main mặc định. PR #2 còn open/not merged, headff8a36a. Local codex/docs-main-workflow cùng origin; trước sửa chỉ VstHelper untracked. Connector list collaborators/invitations trả INVALID_ARGUMENT vì endpoint không hỗ trợ; chưa có bằng chứng list quyền. Không gán đã mời/accept; đã hỏi username/profile của hai bạn qua async, chưa đoán tài khoản.
- research: Đọc GitHub Docs về personal collaborators, quyền repo cá nhân, clone bằng Desktop, GitHub flow, PR review, relative links, Issues/Project và quyền Project riêng. Repo public đọc lesson được ngay; cộng tác ghi cần invitation được accept; Project cá nhân có quyền riêng. Khi có người khác sẵn sàng review mới tăng approvals từ 0 lên 1.
- deliverables: Sổ tay GitHub nhóm với thao tác mời/clone/đăng lesson, một task từ issue đến PR/merge, board và K01–K04; mục lục Lesson 0–3; sửa 94 link nội bộ trong 6 Markdown thành tương đối và #L cho dòng source. Cập nhật README/CONTRIBUTING/templates, kế hoạch/quy trình và bộ nhớ. Không tạo Lesson 4 hoặc sửa C/h/S/Makefile/workflow.
- validation: Đã kiểm tra 19 Markdown thay đổi/mới, 146 relative links tồn tại, code fences cân bằng và không còn link nội bộ kiểu root ổ D: trong nhóm file; git diff --check đạt. Không chạy build/test vì chỉ sửa tài liệu.
- status: Các nội dung mới còn local, chưa tự commit/push, gửi lời mời, tạo/assign issue hoặc board. Các owner/module/lịch trong sổ tay vẫn là đề xuất chờ nhóm nhận; không nhận công việc học trước là đóng góp của Minh/Khánh.
- next action: Có đúng username thì hướng dẫn mời/accept; hai bạn clone và thử một PR ghi chú riêng; chốt board/task và reviewer. Tài liệu mới cần người dùng tự chọn commit/push qua Desktop; không tạo chuỗi commit chỉ để ghi lại mỗi click.

## L025 - 19/10/2026 - Xác nhận bảo vệ main đã đủ hai check bắt buộc

- type: observation / validation / guidance
- verified: Ruleset main-workflow id25416827 active, target ~DEFAULT_BRANCH, bypass rỗng. Required status checks đã có linux và macos, cả hai integration_id15368; strict_required_status_checks_policy=false. PR vẫn 0 approvals, resolve conversations; deletion/non_fast_forward/linear history giữ bật. Cấu hình bảo vệ main cho giai đoạn solo đã hoàn thành, chưa phải nhóm đã thêm thành viên hoặc tạo board/issues.
- local: main cùng origin/main ở lần đọc; 4 file docs/memory modified, kalloc.c có chú thích Linh thêm, VstHelper untracked. Không sửa C hoặc thao tác Git ghi thay người dùng.
- next step: Linh tạo nhánh codex/docs-main-workflow từ main trong Desktop, commit riêng CURRENT_STATE/LOGBOOK/RECOVERY_NOTES và docs/Thiet-lap-GitHub-26118.md với message docs: record verified main workflow; bỏ chọn kalloc.c/VstHelper. Push và mở PR theo quy trình vừa thiết lập. Sau đó quay lại Lesson 3 P02, không mở rộng thêm thủ tục Git không cần thiết.
- documentation: Chốt lại recovery notes ngắn; các trạng thái trước vẫn ở logbook theo ngày. Chỉ thay tài liệu, kiểm tra whitespace đúng 4 file; chưa stage/commit/push/merge hoặc test implementation.

## L024 - 19/10/2026 - Ruleset đã bật, còn thiếu hai check CI

- type: observation / guidance
- verified: GitHub có branch ruleset main-workflow id25416827, enforcement active, target ~DEFAULT_BRANCH, bypass rỗng. Rules deletion, non_fast_forward, required_linear_history bật. PR required_approving_review_count=0, required_review_thread_resolution=true, không require code owner/last-push approval.
- gap: required_status_checks rule tồn tại nhưng required_status_checks=[]; strict=false. Chưa được kết luận PR bị chặn theo kết quả linux/macos. Cần Linh Add checks hai tên đã có thật rồi Save changes và đọc lại.
- local: main53872b8 đồng bộ origin; 4 file docs/memory còn modified từ cập nhật trước. Diff kalloc.c mới chỉ thêm một chú thích trước acquire trong kalloc, không đổi logic; không sửa/format C thay Linh hoặc biến chỉnh chú thích thành điều kiện thiết lập. VstHelper vẫn untracked.
- next action: Hướng dẫn bổ sung checks trong ruleset hiện có, không tạo ruleset trùng; kiểm tra lại cấu hình sau người dùng lưu. Chưa tự thay Settings hoặc stage/commit/push.

## L023 - 19/10/2026 - PR đã merge, main đồng bộ và CI đạt

- type: observation / validation / guidance
- user actions: Linh commit/push 11 file tài liệu thành 2df489d, squash merge PR #1, chuyển GitHub Desktop sang main rồi Pull origin. Hộp thoại Switch branch được hướng dẫn Bring my changes to main sau khi xác nhận chỉ VstHelper/ untracked.
- verified Git: PR #1 merged=true lúc 06:55 UTC; squash commit 53872b8895e82f7baa453c077a0a5255b7310c79, title chore: add page-count diagnostics and team workflow. Local main cùng origin/main; trước cập nhật bộ nhớ chỉ VstHelper/ untracked, không còn tracked diff.
- verified CI: Run PR39210447048 tại 2df489d success. Run main39212699734 tại 53872b8 completed/success; jobs linux và macos success. Linux qua format/build/usertests/crash, macos qua build kernel/fs.img. Không chạy build/test local, chưa có per-CPU/slab/benchmark.
- verified settings: Chỉ squash bật; main.protected=false và rulesets=[] sau merge. Không suy ra đã bật protection chỉ vì main có CI xanh.
- next action: Hướng dẫn Linh tự tạo branch ruleset main-workflow, target main/default branch, enforcement Active; yêu cầu PR với 0 approvals khi solo, require checks linux/macos, giải quyết conversations, linear history, chặn force push/xóa. Chưa áp dụng cấu hình; kiểm tra lại server khi Linh báo xong.
- changes: Cập nhật CURRENT_STATE, RECOVERY_NOTES và tài liệu thiết lập theo mốc đã kiểm chứng; chưa commit các cập nhật này. Không sửa implementation hoặc tự stage/commit/push/merge/Settings.

## L022 - 19/10/2026 - CI thành công và chốt tài liệu chuẩn bị bằng Desktop

- type: observation / validation / guidance
- user request: Linh báo đã xong và hỏi bước tiếp theo.
- CI verified: Plugin đọc run39209117349 completed/conclusion success, updated_at 2026-10-19T06:22:42Z (13:22:42 Asia/Saigon), head58be186120ab6315f7c5f5c830e9e1bd33896af1. Linux117918333408: format/build/usertests/crash đều completed/success. macOS117918333592 success với build kernel/fs.img. Đây là kết quả CI từ xa của checkpoint/PR merge ref, không phải build/test local hoặc benchmark.
- Git verified: Local/origin vẫn58be186, PR #1 open/not merged, 2 commits/20 files. 9 tracked Markdown/config docs modified + 2 new Markdown + VstHelper untracked = 12 mục Changes. Source/Makefile/workflow không có diff; git diff --check đạt.
- next small step: Desktop bỏ tick VstHelper, giữ 11 file docs/quy trình, Summary docs: organize team workflow and GitHub guidance, Commit 11 files rồi Push origin. Mốc commit riêng docs để tiếp tục chuẩn bị repo; không yêu cầu chỉnh chú thích hay sửa C.
- scope note: Giai đoạn solo/chuẩn bị có thể chốt checkpoint ban đầu một lần bằng squash sau review/CI, rồi các task mới mở PR nhỏ. Tách checkpoint thành các PR nhỏ vẫn là phương án nếu cần review độc lập, không là điều kiện phải rewrite lịch sử.
- changes: Cập nhật README/tài liệu audit và bộ nhớ theo kết quả cuối thực tế; không sửa C/h/S/Makefile/workflow hoặc thao tác Git ghi/merge.
- validation: Kiểm tra tài liệu/link và xác nhận 11 file trước hướng dẫn Desktop; không chạy lại test đã đạt khi chưa có source thay đổi.

## L021 - 19/10/2026 - Xác nhận push thành công và CI đã qua format/build

- type: observation / guidance / validation
- user context: Linh gửi ảnh Desktop đã chuyển sang Fetch origin nhưng Changes vẫn 12, nói không có gì xảy ra. Cần giải thích kết quả push và phần local chưa commit bằng ngôn ngữ đơn giản.
- verified push: Git local origin/head cùng 58be186120ab6315f7c5f5c830e9e1bd33896af1, rev-list 0/0. Plugin GitHub PR #1 head cùng SHA, 2 commits/20 changed files, còn open/not merged; merge ref mới 7e9ea6227d841aa32e1f7d30c0771ee43485c138. Không phải thao tác push lỗi.
- CI: Tạo run PR39209117349 và push39209113495, đều in_progress ở lần đọc. Jobs của run PR: linux117918333408 checkout/deps/format/build success, usertests in_progress, crash pending; macos117918333592 success, build kernel/fs.img success. Chưa có kết quả cuối run/usertests/crash; chưa kết luận runtime hay benchmark pass.
- guidance: Push gửi commit đã tạo; các file chưa commit trong Changes vẫn ở máy. Hướng dẫn xem run/PR Checks trên GitHub để thấy tiến độ, tiếp tục dùng Desktop cho commit/push. Không commit nhóm 12 file chỉ để làm sạch danh sách, VstHelper vẫn ngoài scope.
- actions: Chỉ đọc Git/GitHub jobs và cập nhật bộ nhớ; không thao tác Git ghi, không rerun/merge/comment hoặc tác động source.
- next action: Đọc lại run39209117349 khi cần kết quả mới; chỉ chốt mốc/merge sau khi có kết quả thực và scope được review.

## L020 - 19/10/2026 - Hướng dẫn GitHub Desktop và xác nhận commit format

- type: user-preference / observation / guidance
- request: Linh yêu cầu hướng dẫn bằng GitHub Desktop, gửi ảnh repo BTL với Push origin 1, Changes 12 và commit style(kernel): fix formatting vừa tạo.
- verified: Git local HEAD58be186 trên codex/linh-review-existing-changes, origin7a61ccf, rev-list left/right 0/1. Commit chứa đúng bio.c/kalloc.c/main.c/spinlock.c/string.c, 13 insertions/10 deletions. Index rỗng; source không còn diff chưa commit, tài liệu và VstHelper vẫn chưa commit.
- preference: Ưu tiên hướng dẫn GitHub Desktop, tên/vị trí nút cụ thể, một thao tác rõ mỗi bước. Chỉ dùng lệnh terminal cho người dùng khi cần và nêu lý do; không giới hạn công cụ đọc Git của agent.
- guidance: Chỉ cần bấm Push origin trên thanh trên, chờ trở lại Fetch origin. Nút Commit 12 files tạo commit khác từ Changes, không phải nút push. Các thay đổi chưa commit không được đưa lên chỉ bằng push commit format.
- changes: Cập nhật AGENTS, OP-009 và checkpoint; không stage/commit/push hoặc điều khiển UI thay người dùng.
- next action: Sau Linh push, đọc CI run mới; chưa xác nhận remote đã nhận 58be186 hoặc tests pass.

## L019 - 19/10/2026 - Bỏ yêu cầu chỉnh chú thích và tiếp tục chốt Git

- type: user-preference / guidance
- user request: Linh nói không cần coi trọng chú thích code và hỏi bước tiếp. Sau giải thích trước đó quá nhiều bước, ưu tiên chỉ dẫn Git ngắn, mục đích rõ.
- decision: Chú thích không là yêu cầu bắt buộc hay điều kiện chặn mốc đã đạt; chỉ gợi ý khi có giá trị kỹ thuật thật hoặc người dùng yêu cầu. Giữ nguyên quyền tự viết C và tự stage/commit/push của người dùng.
- verified: Nhánh review vẫn HEAD7a61ccf, source diff đúng 5 file format như L018, index rỗng, chưa commit/push phần format. Docs và VstHelper còn ngoài scope. Không có lý do yêu cầu sửa lại comment trước chốt format.
- changes: Cập nhật AGENTS, OP-007, current priority và recovery/lộ trình; không sửa source hoặc thực hiện Git ghi.
- next step: Hướng dẫn một block lệnh cd/add đúng 5 file/commit style/push nhánh review, giải thích ngắn add/commit/push. Sau người dùng push mới đọc CI để xác nhận build/test.

## L018 - 19/10/2026 - Review phần format trước commit riêng

- type: code-review / validation
- request: Linh báo đã format theo hướng dẫn.
- observation: Working diff có đúng 5 file source kernel/bio.c, kalloc.c, main.c, spinlock.c, string.c: 13 insertions/10 deletions, chỉ bỏ khoảng trắng/dòng trống và đổi canh/ngắt dòng. Không thấy đổi logic trong diff đã đọc. Chú thích acquire/release dài vẫn ở cuối dòng làm lời gọi bị tách nhiều dòng, hợp lệ nhưng khó đọc.
- validation: git diff --check đạt; clang-format 21.1.8 --dry-run --Werror toàn kernel/*.[ch], user/*.[ch], mkfs/*.c đạt exit 0. Không sửa source hoặc chạy formatter ghi thay người dùng; chưa build/boot/test/benchmark, chưa có run CI mới được kiểm tra.
- git state: Nhánh codex/linh-review-existing-changes vẫn tại 7a61ccf, index rỗng, source format chưa commit. Tài liệu audit chưa commit và VstHelper/ untracked có Git riêng; cần giữ ngoài commit format.
- guidance: Gợi ý chú thích ngắn trên dòng riêng giải thích khóa bảo vệ việc công bố trang vào freelist; release trở lại một dòng. Sau tự sửa/chạy lại format/check, Linh tự stage đúng 5 file, commit style(kernel): align learning changes with clang-format và push nhánh hiện tại. Assistant không stage/commit/push.
- next action: Sau người dùng chốt/push, đọc CI mới; kết quả local style đạt không thay thế runtime hoặc CI pass.

## L017 - 19/10/2026 - Tiếp tục cấu hình và kiểm tra formatter local

- type: observation / guidance
- request: Linh yêu cầu tiếp tục sau hướng dẫn Settings General.
- observations: GitHub xác nhận squash bật, merge commit/rebase tắt. Không đọc lại auto-delete/default message/rulesets. Ba run mới nhất vẫn như audit, chưa có run mới. Source kalloc.c còn nguyên format cũ; working tree có tài liệu audit chưa commit và VstHelper/ untracked.
- environment: wsl --list --verbose xác nhận Ubuntu WSL2; /etc/os-release là Ubuntu 26.04.1 LTS. command -v thấy make, riscv64-linux-gnu-gcc, qemu-system-riscv64; clang-format not found. Log CI cũ ghi gói clang-format 21.1.6; không suy ra phiên bản toolchain local đã kiểm tra đủ.
- next step: Linh cài formatter trong Ubuntu, đặt chú thích dài lên dòng riêng nếu cần và tự chạy make fmt; xem diff đúng phần C để assistant review. Sau khi phần format được kiểm tra mới chốt commit riêng, người dùng tự commit/push. Chưa có kết quả format/build/test mới.
- changes: Cập nhật trạng thái môi trường và setting đã xác minh; không sửa C/h/S/Makefile/workflow, không cài phần mềm hoặc thao tác Git ghi.
- install follow-up: Linh gửi lỗi apt install clang-format: Unable to locate package. Kiểm tra read-only thấy ubuntu.sources đã gồm main/universe/restricted/multiverse, nhưng index resolute/universe binary-amd64 Packages 0 byte và apt-cache search/policy không có clang-format. Chưa xác định nguyên nhân tải index lỗi; cần output sudo apt update. sudo -n true báo cần xác thực tương tác; không có apt update/install nào do agent thực hiện. Đối chiếu hướng dẫn APT Ubuntu chính thức, không vội thêm kho hay đổi tên gói.
- cache diagnosis: Sau apt update do Linh chạy chỉ Hit, file index universe vẫn 0 byte; cấu hình IndexTargets/amd64 có Packages, nguồn universe đã bật. Metadata InRelease ghi size Packages 76.792.699 byte, curl HEAD HTTPS Packages.xz trả 200/15.960.824 byte. Xác định bản cache local rỗng, chưa biết sự kiện tạo ra cache lỗi. Hướng dẫn loại bỏ đúng một file index rỗng để APT tải lại, không xóa gói đã cài hoặc sửa sources; chưa thực hiện xóa/cập nhật/cài thay người dùng, chưa có kết quả sửa thành công.
- install verified: Linh báo đã cài xong; đọc trực tiếp clang-format --version qua WSL xác nhận Ubuntu clang-format 21.1.8 (6ubuntu1). Chưa có diff kernel/user/mkfs mới; chưa format/build/test. Cập nhật bước tiếp sang Linh tự xử lý chú thích dài rồi make fmt và review diff trước commit riêng. Không suy ra phiên bản local trùng chính xác CI cũ 21.1.6.

## L016 - 19/10/2026 - Audit GitHub và chuẩn bị quy trình solo rồi nhóm

- type: observation / research / documentation
- user request: Dùng plugin GitHub kiểm tra repo hiện tại, việc cần setting cho quy trình ngăn nắp và lịch sử commit; Linh nói chưa thêm thành viên.
- method: Plugin GitHub get_repo/fetch/search PR/issues/branches/commits/rulesets/jobs/job log; Git local log/stat/diff/config định danh và ls-remote tags. Đọc GitHub Docs về rulesets, squash, workflow triggers và commit email. Không đọc nội dung repo lồng VstHelper.
- state: Main c829ab9 có 4 commit, trong đó 8ea6f01/c829ab9 tên dấu chấm; code chú thích/whitespace trộn với docs. Nhánh codex/linh-review-existing-changes@7a61ccf đã push, hơn main 1; PR #1 mở, 17 file, không draft/chưa merge. Đầu audit tracked working tree sạch, VstHelper/ untracked có .git riêng.
- settings: Public/default main; main.protected=false, rulesets=[]; cả merge/squash/rebase bật, auto-delete head branch tắt. Admin endpoint protection bị integration 403, không suy diễn lỗi đó; kết luận bảo vệ dựa branches/rulesets. Issues query rỗng. Projects feature bật nhưng chưa xác nhận board v2. Templates có trong head PR, chưa main.
- history/identity: Đồ thị chưa rối; khó đọc do tên/scope. Global config hiện tên Nguyễn Kim Linh/noreply GitHub đúng, checkpoint mới gắn profile, 4 commit cũ author API null. .gitattributes đã chuẩn LF. Tag baseline local@e287830; API ref 404 và git ls-remote --tags origin rỗng, chưa chia sẻ tag.
- CI evidence: Danh sách 5 run đều conclusion failure; xem run PR 39202934361 và run main 38749137150. Run PR job linux 117899570299 fail make fmt + diff; log diff đúng 5 file bio/kalloc/main/spinlock/string. Linux build/usertests/crash skipped. Job macos 117899570533 build kernel/kernel fs.img success, kiểm tra synthetic PR merge ref 5fef268… từ head7a61ccf/basec829ab9, chưa phải merge thực. Thay kết luận cũ chưa có bằng chứng build, nhưng chưa xác nhận runtime/benchmark.
- proposal: Giữ 4 commit main, dùng nhánh task/PR/squash rõ từ đây; bảo toàn checkpoint rồi tách code/docs khi bàn giao. Solo PR với 0 approval bắt buộc, có thành viên thì 1 review; sửa format và tách job CI trước require checks. Settings chưa apply; chưa rewrite lịch sử.
- outputs: docs/Thiet-lap-GitHub-26118.md, CONTRIBUTING.md; link README tương đối và trạng thái CI thật; templates thích hợp solo/team; bổ sung quy trình/bộ nhớ. Attach PR #1 vào chat để tham chiếu, không gửi comment/review hay request reviewer.
- validation: 9 Markdown và 29 liên kết local hợp lệ, code fences/UTF-8/whitespace đạt; README không còn link root máy cũ. git diff --check đạt; git diff --exit-code trên kernel/user/mkfs/Makefile/workflow đạt, giữ nguyên implementation và CI. Không chạy formatter/build/test local hoặc pipeline mới. Lỗi CI được dẫn từ log thật, không tuyên bố đã sửa/pass.
- actions not performed: Không thay C/h/S/Makefile/workflow, Git config/history/refs/Settings; không stage/commit/push/merge/mời người/tạo issue/board.
- next action: Linh xem checklist, tự sửa format C theo bài học rồi kiểm tra/chốt; áp dụng Settings và tách PR chỉ khi được yêu cầu rõ. Cuối tuần điền kết quả thật vào W01, không dùng trạng thái audit cũ để kết luận CI hiện tại.
- should promote to current state?: yes, facts và kết quả audit; cấu hình/cleanup giữ nhãn đề xuất.

## L015 - 19/10/2026 - Nghiên cứu quy trình nhóm 26118 và báo cáo tuần

- type: user-context / research / documentation
- summary: Người dùng xác nhận tên Linh, muốn tham gia nhiều/hiểu toàn bộ và chia phần riêng cho ba thành viên như cách làm việc thực tế; cung cấp ảnh đề tài, thông báo, template PDF và sample PDF.
- facts: Mã 26118, đề tài 19; Nguyễn Kim Linh 20233495, Lê Trần Hiếu Minh 20233528, Nguyễn An Khánh 20224019. Báo cáo ít nhất mỗi tuần, cùng thread môn học, mọi người có đóng góp, nối TODO trước và cho phép sai khác 25%; template có năm phần, summary 200–300 từ.
- user clarification: Trong lúc nghiên cứu, Linh xác nhận tuần này mới bắt đầu dự án nhóm, phần đang học là học trước. W1 theo lịch là 19–25/10/2026; không gộp Lesson/code/tài liệu chuẩn bị cá nhân thành kết quả nhóm W01. Đã tách task khởi động K01–K04 khỏi TODO W2 N01–N04 có điều kiện đầu vào.
- method: Đọc chat được chỉ định qua read_thread; đọc memory/kiến trúc/source/Git/CI; trích và render hai PDF; xem ảnh; kiểm tra tài liệu chính MIT lock lab, Bonwick/USENIX, GitHub Flow/Projects/CODEOWNERS và QEMU MTTCG.
- observation: Source vẫn list/lock chung với kfreepage_count/prototype/caller; chưa perCPU/slab. Không có user/kalloctest.c hoặc kernel/kcsan.c. CI file có nhưng không kiểm chứng pipeline. Baseline tag e287830, HEAD c829ab9, working tree có thay đổi từ trước; remote kimsakota/BTL.
- proposal: Một repo/ba clone, nhánh ngắn theo task, API nhỏ, PR/review chéo. Linh page/buffer/tích hợp, Minh slab/file, Khánh test/inode; Linh học toàn luồng bằng review/teach-back/reproduce. Slab phát triển trên page gốc để song song; ba client chính file/inode/buf, pipe thử trước. Chưa được nhóm chốt phân công.
- outputs: docs/Quy-trinh-nhom-26118.md; docs/Ke-hoach-va-backlog-26118.md; docs/bao-cao/README.md; bản nháp W01 Markdown/PDF; mẫu task/PR local .github; liên kết README và bộ nhớ.
- report status: PDF ba trang, summary 251 từ theo whitespace; kiểm tra lại bản render sau khi cập nhật bối cảnh khởi động. Ngày from/to thực tế, đóng góp thực Minh/Khánh, xác nhận phần của Linh và URL design/board còn thiếu; nhãn DRAFT rõ ràng, không nộp thay nhóm. Không lấy tiến độ/test/công cụ nhóm mẫu làm dữ kiện nhóm này.
- validation: Kiểm tra 10 Markdown, 21 liên kết local, code fences và đối chiếu ID K01–K04/N01–N04; summary 251 từ, PDF đủ mục/MSSV, ba trang, text bounds hợp lệ; xem cả ba PNG bản cuối. git diff --check tài liệu đạt. Diff kernel/user/mkfs/Makefile giống đầu lượt và SHA256 kalloc.c/defs.h/main.c/Makefile giữ nguyên. Không chạy build/boot/test/benchmark.
- limitations: Đã biết tuần khởi động nhưng chưa biết giờ nộp chính thức/hạn cuối kỳ; đề xuất tổng hợp nội bộ 24/10 chỉ là lịch nhóm. W01 còn là khung chờ đầu ra cuối tuần. Công thức sai khác 25% chưa do giảng viên xác nhận; không tự gán phần trăm tiến độ. Mẫu/plan không chứng minh ba thành viên đã đóng góp.
- actions not performed: Không sửa implementation C/h/S/Makefile, không stage/commit/push, không tạo board/issue/PR hoặc gửi tin/báo cáo ra ngoài.
- next action: Nhận kỳ/hạn và contributions thật, chỉnh bản nháp theo đúng thời gian; hoặc tiếp tục tự code Lesson 3 khi Linh quay lại học.
- should promote to current state?: yes, chỉ thông tin nhóm/yêu cầu/đầu ra, phân công giữ nhãn đề xuất.

## L014 - 17/10/2026 - Tạo Lesson 3 về free list theo CPU

- type: documentation / observation
- summary: Người dùng yêu cầu file Markdown Lesson 3; giữ cách dạy đầy đủ, tự viết C, gợi ý chú thích và nhắc commit/push từng phần.
- source update: Kalloc.c đã có kfreepage_count với lock bao vòng duyệt, defs.h có prototype, main.c gọi/in ngay sau kinit. Đây là bằng chứng source, chưa xác nhận runtime; thay kết luận cũ chưa có hàm đếm.
- result: Tạo docs/lessons/03-per-cpu-free-lists.md với pool/mảng struct, lifetime tên khóa, init trước freerange, ownership/invariant, guard/local free-alloc, count nhiều pool và bản đồ reference. Có bài A–I và mốc M0/M1/M2 để người dùng tự code/check/commit.
- design for lesson: M1 đổi state thành mảng nhưng giữ pool 0 chung. M2 local-only chưa stealing, chỉ đặt tiêu chí CPUS=1; local-empty không là global OOM. Lesson 4 thêm remote allocation trước kiểm chứng nhiều CPU.
- count semantics: Giữ một khóa mỗi lần quét pool. Boot yên cho tổng giải thích theo end/PHYSTOP; runtime không coi tổng nhiều thời điểm là snapshot đồng thời. Chưa thêm counter chung hoặc giữ đồng thời nhiều khóa.
- sources: Source BTL/Makefile và kiến trúc, MIT lock lab/xv6 book, WG14 N1570. Thiết kế M1/M2 là hướng dẫn cho BTL, không phải implementation đã nhập.
- changes: Lesson 3, liên kết từ Lesson 2/README/lộ trình và bộ nhớ; không sửa C/header/assembly/Makefile, không stage/commit/push.
- validation: Kiểm tra 7 tài liệu, 68 liên kết local/số dòng, code fences, thứ tự mục 0–15 và 3 điểm source của hàm đếm/khai báo/caller; git diff --check tài liệu đạt. Diff kernel/user/mkfs/Makefile sau soạn bài giống snapshot đầu lượt, giữ nguyên phần C người dùng đã viết.
- limitations: Chưa kiểm tra lại cài môi trường; chưa build/boot/test. Chưa có source per-CPU/slab. Việc tạo tài liệu không tự hoàn thành các mốc code.
- next action: Người dùng kiểm tra/chốt hàm đếm nếu chưa làm, rồi tự viết pool/mảng và lần lượt M1/M2; assistant kiểm tra, gợi ý chú thích/message và nhắc tự commit/push.
- should promote to current state?: yes

## L013 - 15/10/2026 - Gợi ý chú thích và nhắc commit sau từng phần code

- type: user-decision / documentation
- summary: Người dùng yêu cầu thêm quy tắc: với phần hướng dẫn có sửa code, sau khi xong một phần người dùng sẽ commit lên GitHub; assistant thêm gợi ý chú thích và lời nhắc.
- decision: Mỗi checkpoint code có ý nghĩa nêu thay đổi/kiểm tra thật, gợi ý chú thích giải thích lý do và hợp đồng, đề xuất commit message phù hợp, chỉ rõ file/hunk và nhắc tự commit local rồi push remote. Kể cả sửa chú thích source cũng thuộc quy tắc; bài chỉ làm trên giấy/chat chưa thay file không có mốc commit code.
- ownership: Người dùng tự viết chú thích/code và tự stage/commit/push. Assistant chỉ hướng dẫn/nhắc, không thực hiện Git write/publish khi chưa có yêu cầu rõ.
- changes: AGENTS.md, quyết định OP-007, recovery, mục chốt phần code trong lộ trình và phần gợi ý chú thích/commit của Lesson 1/2.
- limitations: Lượt này chỉ lưu quy tắc/cập nhật tài liệu, không sửa C và không stage/commit/push.
- next action: Áp dụng quy tắc khi kiểm tra một phần code do người dùng hoàn thành hoặc soạn lesson có phần tự viết.
- should promote to current state?: yes

## L012 - 15/10/2026 - Tạo Lesson 2 về đồng bộ, CPU và ngắt

- type: documentation / observation
- summary: Người dùng yêu cầu tạo bài tiếp theo sau Lesson 1; giữ cách soạn Markdown đầy đủ và hướng dẫn tự viết từng phần.
- result: Tạo docs/lessons/02-race-spinlock-va-cpu.md với race/lost update, critical section/invariant, atomic/spinlock/acquire-release, ordering/volatile, ngắt/CSRRC, noff/intena có mức lồng, cpuid/mycpu/migration, deadlock/lock order, định hướng per-CPU và helper quan sát tự viết. Có bảng, sơ đồ, bài phân tích A–F, source map và nguồn nghiên cứu.
- sources: spinlock.c/.h, proc.c/.h, riscv.h, start.c, trap.c, kalloc.c, main.c, sleeplock.c và Makefile trong BTL; MIT xv6 book/lock lab, GCC atomics/volatile, WG14 N1570 và RISC-V ISA.
- source observations: push_off dùng rc_sstatus đọc/xóa SIE; diff source hiện tại ở kalloc.c/proc.h chỉ thêm chú thích. user/kalloctest.c và kernel/kcsan.c không có; không áp lệnh test lab như đã sẵn sàng tại BTL.
- changes: Lesson 2, liên kết từ Lesson 1/README/lộ trình và bộ nhớ. Không sửa implementation hoặc thư mục .vs/.
- validation: Kiểm tra 7 tài liệu, 73 liên kết local/số dòng, code fences, thứ tự mục 0–15 và 6 đoạn source atomic/push_off/holding/mycpu; git diff --check tài liệu đạt. Diff kernel/user/mkfs/Makefile sau soạn bài giống snapshot đầu lượt, giữ nguyên các thay đổi của người dùng.
- limitations: Chưa có bài làm/helper mới hoặc kết quả build/boot/test; không kiểm tra lại cài môi trường. Yêu cầu tạo bài tiếp theo không tự xác nhận đã hoàn thành runtime Lesson 1.
- next action: Người dùng đọc Lesson 2 theo chặng, gửi bài đang làm hoặc yêu cầu Lesson 3; kiểm tra/dạy theo điểm thực tế, giữ quyền tự viết C của người dùng.
- should promote to current state?: yes

## L011 - 14/10/2026 - Soạn Lesson 1 đầy đủ sau Lesson 0

- type: user-decision / documentation
- summary: Người dùng xác nhận đọc xong Lesson 0; yêu cầu nghiên cứu và tạo bài Lesson 1 Markdown đầy đủ, giải thích chi tiết từng phần. Không tiếp tục giữ bài học ở khai báo hàm Lesson 0.
- result: Viết lại/mở rộng file Lesson 1 hiện có, giữ đường dẫn 01-baseline-va-kalloc.md để các liên kết cũ tiếp tục dùng. Nội dung gồm byte/địa chỉ, kiểu/typedef/ABI, pointer/&/*, cast/sizeof/bước pointer, struct/./->, list/metadata, source map và từng hàm allocator gốc, ownership/khóa, môi trường/baseline, bài nhỏ A–E và hàm đếm theo chặng.
- sources: Source BTL và Makefile thực tế; MIT xv6 book/tools, WG14 N1570, RISC-V psABI, GCC Standards/Pointer Arithmetic, Microsoft WSL. Nguồn và phạm vi được ghi trong mục 15 của Lesson 1.
- operating preference: Cung cấp tài liệu đủ cả bài khi chuyển lesson; việc tự code vẫn chia từng bước nhỏ, assistant chỉ giải thích/kiểm tra/gợi ý.
- changes: Lesson 1, liên kết tiếp tục từ Lesson 0, README/lộ trình và bộ nhớ. Không thêm/sửa C/header/assembly/Makefile.
- validation: Kiểm tra Markdown/code fences, thứ tự 16 mục 0–15, 44 liên kết local và giới hạn số dòng source trong các tài liệu liên quan; git diff --check phần tài liệu đạt. Diff kernel/user/mkfs/Makefile sau soạn bài giống snapshot trước, giữ nguyên thay đổi chú thích của người dùng.
- limitations: Đã đọc xong Bài 0 là lời xác nhận của người dùng, không phải kiểm tra thành thạo; chưa có bài làm Lesson 1, chưa build/boot/test và chưa kiểm tra lại môi trường cài đặt.
- next action: Người dùng đọc Lesson 1 theo chặng, gửi bài nhỏ đang làm; assistant đọc và hướng dẫn tiếp theo điểm thực tế.
- should promote to current state?: yes

## L010 - 14/10/2026 - Tiếp tục hợp đồng hàm và khai báo C

- type: learning-note / observation
- summary: Người dùng yêu cầu tiếp tục kiến thức sau phần phân biệt mã xv6 và phần đề tài.
- covered topics: Trách nhiệm, tham số, kết quả và trạng thái phụ thuộc của kalloc; khai báo ở defs.h, định nghĩa ở kalloc.c; phân biệt void, void* và (void).
- exercise: Tự viết một dòng khai báo cho hàm dem_trang_trong, không nhận tham số, trả int; gửi trong chat. Đây là bài tập cú pháp, không phải API đề tài đã chốt, không thêm vào source.
- source checked: git diff kernel/defs.h và kernel/kalloc.c chỉ có chú thích/khoảng trắng; phần allocator chưa đổi. Không sửa các thay đổi đó thay người dùng.
- changes: Bổ sung hợp đồng hàm và bài tập vào Bài 0, cập nhật điểm tiếp tục/bộ nhớ.
- limitations: Chưa có bài làm để đánh giá mức độ hiểu; không kiểm tra lại môi trường hoặc chạy build/boot/test trong lượt này.
- next action: Kiểm tra một dòng khai báo do người dùng tự viết, rồi giải thích header/include và compile/link.
- should promote to current state?: yes

## L009 - 13/10/2026 - Phân biệt mã xv6 nền và phần tự viết của đề tài

- type: learning-note / observation
- summary: Người dùng hỏi vì sao kernel/ có nhiều code sẵn và có phải mã quản lý đề tài hay không.
- explanation: kernel/, user/, mkfs/ và Makefile đến từ xv6 nền; docs/ và agent/ phục vụ tài liệu/bộ nhớ. Đề tài mở rộng quản lý bộ nhớ trong nhân xv6; người dùng tự viết phần mở rộng và tích hợp, sử dụng hệ điều hành nền có sẵn.
- evidence: docs/Nguon-xv6.md, kernel/kalloc.c và git diff ở kernel/user/mkfs/Makefile không có thay đổi so với baseline local tại lần kiểm tra này.
- changes: Bổ sung mục 0 vào Bài 0, cập nhật điểm tiếp tục. Không sửa implementation.
- limitations: Mới giải thích ranh giới mã nền/phần đề tài; chưa kiểm tra người dùng đã nắm. Slab/per-CPU free lists chưa được triển khai; chưa build/boot.
- next action: Đọc mục 0 trước khi học khai báo/định nghĩa hàm bằng source tham chiếu.
- should promote to current state?: yes

## L008 - 13/10/2026 - Làm rõ cách dạy và tự viết C

- type: user-decision
- summary: Người dùng yêu cầu tự viết toàn bộ phần C và hiểu tất cả reference/phụ thuộc, liên kết từng thành phần từ nhỏ đến cần thiết.
- correction: Đưa một hàm hoàn chỉnh để gõ theo quá sớm không đáp ứng mục tiêu tự thiết kế/hiểu. Assistant nhận lỗi cách hướng dẫn và chuyển sang giao từng bước nhỏ.
- decision: Assistant giải thích khái niệm, hợp đồng, kiểu dữ liệu, khai báo/định nghĩa, header/include và compile/link; người dùng tự viết và tự sửa. Assistant chỉ đọc/kiểm tra/gợi ý, không sửa implementation hay đưa lời giải toàn bộ nếu chưa được yêu cầu rõ.
- changes: Cập nhật AGENTS.md, lộ trình và bài tập trong Bài 1; bỏ lời giải hàm đếm và các dòng tích hợp hoàn chỉnh. Chỉ sửa tài liệu/bộ nhớ, không thay mã C.
- next action: Bắt đầu từ vai trò khai báo trong defs.h, định nghĩa trong kalloc.c và reference từ main.c; học con trỏ/struct rồi mới tự viết từng phần của hàm nhỏ.
- should promote to current state?: yes

## L007 - 13/10/2026 - Bắt đầu học và code theo từng bài

- type: user-decision / implementation / learning-note
- summary: Người dùng yêu cầu hướng dẫn chi tiết A-Z, vừa học vừa hiểu vừa code; chọn tự gõ và để assistant hướng dẫn/kiểm tra.
- preparation: Nhập 81 file source/cấu hình được Git theo dõi từ xv6 Exercise vào root BTL, chuẩn hóa LF và giữ sửa đổi Makefile. Khởi tạo Git local và lưu baseline bằng tag xv6-baseline.
- evidence: docs/xv6-baseline.json, Git baseline, docs/Lo-trinh-hoc.md và Bài 1.
- observed environment: Kiểm tra lại WSL cho thấy chưa có distro; chưa xác nhận toolchain hoặc build/boot.
- lesson prepared: Đọc kinit/freerange/kfree/kalloc; thực hành tự gõ kfreepage_count, prototype trong defs.h và printk sau kinit.
- limitations: Code bài tập nằm trong hướng dẫn, chưa được thêm vào kernel. Chưa có kết quả runtime hoặc usertests.
- next action: Người dùng thiết lập Ubuntu/toolchain, chạy baseline gốc; sau đó tự gõ và gửi kết quả để đọc diff/kiểm tra.
- should promote to current state?: yes, chỉ các thao tác chuẩn bị và lựa chọn đã xác nhận.

## L006 - 13/10/2026 - Tích hợp bộ nhớ agent

- type: decision / implementation
- summary: Người dùng yêu cầu thêm một thư mục agent từ GitHub để quản lý lịch sử, nội dung và quy tắc.
- context: BTL nhỏ, đang học C/xv6, đã yêu cầu cấu trúc vừa đủ.
- result: Chọn project-memory-skill cấu hình Light theo đánh giá phù hợp của assistant; tích hợp tài liệu ở `agent/` và router ở root `AGENTS.md`. Đây là lựa chọn triển khai trong phạm vi yêu cầu, không phải tuyên bố repo được cộng đồng khuyên dùng nhiều nhất.
- evidence: `UPSTREAM.md`, `README.md`, `history/INDEX.md`.
- limitations: Bộ nhớ được agent duy trì bằng file; chưa cài runtime hooks/MCP hoặc đồng bộ chat liên tục. Chưa có Git repo/commit tại root BTL.
- next action: Dùng recovery/current state trong các chat tiếp theo; cập nhật theo trách nhiệm.
- should promote to current state?: yes

## L005 - 13/10/2026 - Kiểm tra source xv6 được cung cấp

- type: observation
- summary: Người dùng cung cấp bản xv6 ở Exercise/4/xv6-riscv và hỏi có dùng được không.
- method: Đọc source, Makefile, Git status/remote/log/diff; xác nhận commit trên repo MIT.
- result: Phù hợp làm nền đề tài 19; commit `06aad25c735fd3159bdfae5680be4eab7b1668b2`; allocator trang chung, giới hạn 8 CPU, cấu trúc file/inode/buffer tĩnh.
- confidence: cao về source đã kiểm tra.
- limitations: Chưa build/boot mới; chưa import vào BTL. File build có sẵn không chứng minh runtime hiện tại.
- source: `docs/Nguon-xv6.md`; snapshot chat hiện tại trong history.
- next action: Khi bắt đầu implementation, tạo baseline riêng và xác nhận build/boot.
- should promote to current state?: yes

## L004 - 13/10/2026 - Dựng workspace và định hướng cấu trúc C

- type: decision / observation
- summary: Người dùng yêu cầu cấu trúc hợp lý, không thừa thãi, hỏi về Clean Architecture; xác nhận dựng khung trước vì chưa có starter tại BTL.
- result: Dựng workspace, task build/run/clean, README và hướng dẫn. Thiết kế đề xuất giữ module C theo cấu trúc xv6: kalloc.c cho trang, kmalloc.c/.h cho object.
- method: Kiểm tra cấu hình JSON, đường dẫn và tài liệu; đọc Makefile xv6 chính thức.
- observation: Lúc kiểm tra có Git, VS Code, wsl.exe; chưa có distro WSL. Không tìm thấy toolchain trong Windows PATH.
- limitations: Không build kernel; sơ đồ file C là cấu trúc đích.
- source: `docs/Kien-truc-du-an.md`, `.vscode/tasks.json`, `BTL.code-workspace`.
- should promote to current state?: yes, với ngày kiểm tra môi trường.

## L003 - 13/10/2026 - Nghiên cứu hướng triển khai và đánh giá

- type: research
- summary: Đối chiếu catalogue, mã VstHelper và tài liệu slab/magazines/Hoard/TCMalloc/MIT.
- result: Phân biệt per-CPU page lists với per-CPU object cache. Adaptive cache sizing đã có prior work; chưa thể coi “thêm thích ứng” là tính mới.
- proposed direction: Đo batch stealing 1/4/16/64 trên 1/2/4/8 CPU; tải lệch/đổi pha; khảo sát reclaim slab rỗng; buddy ở mốc stretch.
- hypothesis: Adaptive batch có thể cải thiện trade-off ở một số tải. Chưa thử nghiệm, chưa biết có tốt hơn fixed baseline hay không.
- source: `docs/Nghien-cuu-de-tai-19.md`.
- limitations: Không có implementation hoặc benchmark; chưa xác lập khoảng trống nghiên cứu qua systematic review.
- should promote to current state?: Chỉ promote kết quả khảo sát có nguồn, không promote giả thuyết hiệu năng.

## L002 - 13/10/2026 - Kiến thức đã trao đổi

- type: learning-note
- summary: Người dùng muốn học lý thuyết trước, rồi yêu cầu tiếp tục kiến thức.
- covered topics: Kernel, allocator, allocation/free, trang 4 KB, object, slot, slab cache, free list, lock, per-CPU lists và stealing.
- covered mechanics: Metadata, con trỏ giữ địa chỉ, bitmap có quy ước bit trống/đã cấp, tái sử dụng slot, phân biệt cấp phát với khởi tạo.
- covered lifecycle: Slab empty/partial/full; free object khác free trang; empty slab có thể giữ lại hoặc reclaim.
- covered errors: Memory leak, use-after-free, double free; pointer còn địa chỉ sau free không giữ quyền dùng vùng nhớ.
- source: Snapshot chat “Tiếp tục đề tài NCKH 19” trong `history/INDEX.md`.
- limitations: Đã giải thích không đồng nghĩa đã đánh giá mức độ nắm kiến thức của người dùng.
- next suggested lesson: Đọc kalloc.c gốc, hoặc giải thích lock/race condition khi hai CPU cùng cấp phát; theo yêu cầu tiếp theo của người dùng.
- should promote to current state?: yes, chỉ ghi tiến độ bài đã trao đổi.

## L001 - Trước 13/10/2026 - Chốt đề tài 19

- type: user-decision
- summary: Người dùng đã chọn Slab allocator and per-CPU free lists, muốn kế thừa kiến thức BaoKhoaHoc/VstHelper.
- context: Chat “Chọn đề tài NCKH kế thừa BaoKhoaHoc” và yêu cầu mở đầu chat hiện tại.
- result: Tiếp tục đề tài 19; kế thừa thuật toán/thiết kế từ C# unsafe sang C/xv6, không giả định dùng lại nguyên mã .NET.
- source: Snapshot chat được tham chiếu trong `history/INDEX.md`.
- limitations: Đổi nhãn 2 sao thành 3 sao chưa được giảng viên xác nhận. Mở rộng có trọng tâm vẫn là hướng đề xuất.
- should promote to current state?: yes

## Mẫu ghi thêm

Mỗi mục cần ID, ngày, loại, kết quả, nguồn, giới hạn và bước tiếp theo nếu thay đổi. Loại có thể là `user-decision`, `decision`, `observation`, `research`, `hypothesis`, `learning-note` hoặc `blocker`. Giả thuyết được giữ dưới nhãn riêng cho đến khi có bằng chứng.
