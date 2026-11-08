# Trạng thái hiện tại

Cập nhật: 09/11/2026. Checkpoint này là hiện trạng; các mục bên dưới giữ làm lịch sử theo ngày.

## W04 đã tạo trên GitHub; chuẩn bị giải quyết conflict PR #35

- Theo yêu cầu Linh, đã tạo tuần W04 (09–15/11): [Issue cha #36](https://github.com/kimlinh25/BTL/issues/36), milestone #3 trên kimlinh25/BTL và cập nhật Project 1. Task mới: Linh P05 #37; Minh S04a #38 (chỉ sau #27 đạt và còn giờ); cả nhóm D01-W04 #39. Giữ #27 Minh / #28 Khánh / #29 cả nhóm khi chuyển tiếp, không tạo bản sao.
- Hạn nội bộ: #27/#28 PR sửa 11/11, chốt 12/11; #29 bổ sung minh chứng W03 10/11; P05 thiết kế 10/11, PR 12/11, log 14/11; S04a target 14/11 có điều kiện; báo cáo W04 15/11. Quỹ kế hoạch 10–12 giờ/người đã xác nhận trước đây, gồm học/review/báo cáo; assign chưa là xác nhận nhận việc.
- Đã gán đúng assignees và kiểm tra lại 30 điều kiện API: week W04, status, ngày, reviewer, milestone và cha–con. #37/#38/#39 là con #36; #26–#29 vẫn là con W03 #25. #27/#28 chuyển milestone W04; #29 giữ milestone W03 vì chốt báo cáo W03 nhưng có nhãn/tuần W04 để theo dõi phần bổ sung. Evidence: C:/Users/KimSakota/.codex/tmp/BTL-W04-tasks-20261109/verification.json.
- Main GitHub tại lần đọc 09/11 vẫn 9d6d145; PR #32/#33/#34 đã merge, object bản ghép 08/11 vẫn 15 PASS/2 FAIL. PR #35 `update`@3d86914 chỉ tài liệu/bộ nhớ W01–W02, chưa có fix slab/test. Không gọi lõi toàn PASS hoặc P05/S04a đã làm. Tồn đọng #27: fill 0/1, panic modes và thiết kế; #28: biên/mixed 64/128, capacity theo từng class, matrix/log. W04 ưu tiên lõi và P05; class client/reclaim và file/inode/buf xem lại từ W05, giảm dự phòng so với lịch cũ.
- Linh vừa yêu cầu khắc phục conflict PR. Local đang `update`@3d86914, origin/main9d6d145; hai file agent bị unmerged khi khôi phục stash, không có MERGE_HEAD. `git merge-tree` còn xác nhận PR #35 xung đột đúng hai file này với main. Đã backup working files, index và đủ ba stage blobs tại C:/Users/KimSakota/.codex/tmp/BTL-PR35-conflict-20261109/.
- Chọn bản stage3 vì đối chiếu cho thấy nó giữ nguyên toàn bộ main và chỉ thêm 157 dòng checkpoint W03; không bỏ ghi chú mới để lấy snapshot cũ 24/10. Bổ sung checkpoint W04 này. Ngày 09/11 Linh đã cho phép rõ stage hai tệp agent, commit bản hòa, ghép main và push riêng nhánh update để cập nhật PR #35; quyền này không mở rộng sang merge PR vào main hoặc commit báo cáo Word. Giữ nguyên báo cáo W03 chưa track, stash và implementation C/h/S.


## Ghi chú W03 đã lưu — các mục có ngày cũ là lịch sử

## Báo cáo W03 đã tạo và chốt phần tồn đọng sang W04 — 08/11/2026

- Linh yêu cầu tạo báo cáo W03 vì hết ngày, xác nhận các việc chưa hoàn thành làm bù tuần sau. Đây là quyết định chốt kỳ: lập/nộp báo cáo trung thực với kết quả hiện có, không trì hoãn tạo báo cáo để đợi sửa hết lỗi. Các S03a/T03/D01 còn thiếu giữ ID và owner sang W04 (09–15/11); ưu tiên hoàn tất tồn đọng trước khi mở P05/S04/reclaim/client mới.
- Đã tạo docs/bao-cao/26118-Progress-Report-W03-2026-11-08.docx, bốn trang Letter, đủ5mục theo mẫu W02, Summary249từ, đủ ba người/MSSV và15đích hyperlink. Font Times New Roman/toàn bộ chữ đen, kể cả hyperlink/footer. Period02–08/11, nêu thời gian làm04–08/11. Có bảng đối chiếu TODO W02→W03 và W04 phân công Minh/Khánh/Linh cùng nhóm.
- Nội dung theo main9d6d145/baPR32–34 đãMERGED. Báo cáo giữ đúng page12/0, slabPASS, boot1/2 vàusertests-q2PASS; object bảnghép15PASS2FAIL vì65/128expectedNULL. Nêu rõ phần fill0/1/testpanic/design củaMinh và contractmixed64/128/O05củaKhánh chưa xong; không ghi đã khắc phục hoặc toàn PASS. Phân biệt log tự chạy của sinh viên với các review/integration doCodex; không dựng sốgiờ/%/benchmark/concurrent/fullmatrix hoặc xácnhậnnộp.
- Dùng đúng W02 đãfixfont làm designauthority, lấy bảncopy nguyênbyte từcodex/save-docs-20261107; referenceSHA2ce095e8c05088cd39fcc375fe243f4ec98c27501da983b4257bb0d162d655b0 giữ nguyên. OutputSHA256 `62b0f64b9c570f052a7e7768614ab166d894c3d47d47ec25e96063b35e6dcb5a`. Phầnstyles/numbering/theme/fontTable/sectPr/opaqueparts giữbyte; chỉbody/links/footerweek/coreprops thay.
- QA C:/Users/KimSakota/.codex/tmp/BTL-W03-report-20261108/ gồm artifact.md, reference-inventory.json, build_report.py, build-manifest.json, qa-manifest.json, render-v3. Packaged render_docx.py đã chạy nhưng thiếu soffice.exe; fallback WordCOMẩn xuấtPDF vàPopplerbundled tạoPNG. Đã xem đủ4trang bảncuối, không tràn/cắt bảng; PDF chỉTimesNewRoman regular/bold/italic; auditfont/màu/5mục/period/MSSV/links/PAGE1–4/preserve-onlyparts đềuđạt. Chỉ giao DOCX, không giao PDF/PNGQA.
- File DOCX mới chưa commit/push/nộp; #29 lúc đọc vẫnchưa cócommentnộp. Local vẫncodex/w03-page-stealing@79d997c, C sạch, memorydirty vàDOCXuntracked. Chưa sửa C/h/S, đổi nhánh/Pull, stage/commit/push hay đăngcomment/nộp thay. Tiếp theo Linh nộp file và lưu thờiđiểm/permalink tại29; W04 xửlý tồn đọng theoTODO, không sửa lịch sử W03 để xóaFAIL.

## Sau khi Linh merge cả ba PR W03 — 08/11/2026

- Đã xác nhận PR32/33/34 đều MERGED. Merge commits lần lượt5c91ff194fb6c6629a40bcecaeb08ca73b9d06d6,2e2233064e47563dd9bc48f87949cf03d46dc5fc,9d6d145a5f8d2b2a16717aaf2391b98f61bbce1c. Main thực tế9d6d145. Không giữ nhận định baPRcònOPEN bên dưới làm hiện trạng. Issue29 vẫnOPEN/chưacomment tổng hợp hoặc minhchứng nộp.
- Quan trọng: PR32 merge nguyên head988b60a, slab.c trênmain blob2344c6ca63fea8b99ee4df029ccb6607f7364d44, tức chưa có zero-fill/junk1 và SLAB_TEST_CASE handlers như comment Minh nói đã sửa. PR33 merge e7b4de0 sửa đủba lỗi harness nhưng vẫn contractslab64, bounds65/128expectedNULL, workload chỉ1..64 vàO05hardcode63. Hai classS03a đãvào main nên cần chỉnh hợp đồng test/mixed64-128/capacity31, không chỉ đổi expected.
- Codex kiểm tra nguyên snapshotmain9d6d145 ngoài workspace, hai cleanbuild CPUS1/2 vớiKALLOCTEST1 SLABTEST1 đều exit0. Page12PASS0FAIL/bảo toàn vàslabS03a ALL PASSED/shell cảhai. Object cảhai15PASS2FAIL doB01-size65/128want0actual1. CPUS2 usertests-q ALL TESTS PASSED (~59.29s cảboot/regression). verification_completed=true nhưng all_harnesses_pass=false, không tuyên bố bản ghép đạt W03.
- Evidence/runner C:/Users/KimSakota/.codex/tmp/BTL-W03-integrated-20261108/ (check_main.py/results.json/build-cpu1/2.log/cpu1/2.log). Người chạyCodex, revision9d6d145pinned, không gán cho sinhviên. Fetch/archivereadonly, không đổi nhánh/Pull/sửa C/h/S hoặc commit/push/đăngcomment.
- Việc tiếp: Linh tự cập nhật main bằng VisualStudio và giữ bộnhớdirty; Minh tựsửa slab0/1/testpanic/design trên nhánhmới từmain, Khánh tựđồngbộT03contract2class/mixedsizes/O05theo63/31/testmatrix trên nhánhmới. Hai phầnfile khácnhau cóthể làm song song, tựcommit/push/PR. Sau ghépfix, kiểmtra cùng revision boot1/2/page-object-slab/usertests-q2, lognormalđềuFAIL0; gompermalink26/27/28vào29, báo cáo5mục TimesNewRoman/toànđen vàbằngchứngnộpthật. Kết quả cònfail nếuđếnhạnphảighi RemainingIssues/TODO, khôngtickđạtgiả.
- Local hiện vẫn codex/w03-page-stealing@79d997c/Csạch, chỉagent/CURRENT_STATE vàRECOVERYdirty. Agent chưa chuyểnmain hoặcPull thayLinh. P05/fullCPUstress,S04/reclaim/client vẫnW04; khôngcầnrevert/đụng lịch sửmerge chỉđểsửa những điểm này, và chưa thực hiện bất cứrevert nào.

## Audit W03 toàn nhóm — cập nhật mới 08/11/2026

- Linh hỏi cả nhóm đã xong W03 chưa. Đã đọc lại Issues25–29, các comment26–29, PR32/33/34, timeline/reviews và remote main. Cả5Issue vẫn OPEN; #29 chưa comment tổng hợp hoặc permalink báo cáo/nộp. Cả3PR OPEN/chưa merge, review submissions đều rỗng ở lần đọc cuối. Main thực tế vẫn b276acd (chưa ghép codeW03). Không suy việc offline chưa diễn ra, chỉ kết luận chưa có ghi nhận đủ trên GitHub.
- Linh: PR34 head79d997c đã có P03/P04, body ghi đúng log Codex trên bản làm việc trước commit. CI42416578573 SUCCESS; kết quả kỹ thuật tuần này đã kiểm tra gồm page1/2CPU12/0, local-miss có kiểm soát và usertests-q2CPU PASS. Issue26 tickP03/P04, chưa có comment minh chứng; còn ghi nhận review Minh, merge và nối bằng chứng ở26/29. Chưa tự đóng issue hoặc approve/merge.
- Minh: PR32 vẫn head988b60a, tức nguyên bản còn mất zero/junkfill và test panic đã xác nhận. Minh có comment6718027477 nói đã khôi phục và cập nhật docs, nhưng revision vẫn placeholder <mã_commit_mới_nhất>; không có commit/code mới khớp lời mô tả trong PR hoặc remote branch đọc được. Cần push sửa thực, điền revision/log rồi Linh review lại; chưa tính N02 hoàn tất. Không suy đoán code local của Minh chưa làm, chỉ đánh giá nguồn đã đẩy lên.
- Khánh: PR33 đã lên `e7b4de09f9ec7490757fe2120aa25d5c23434d04` (2commit sau0556), comment6719934915 kèm log08/11/inject3/4. Đã đọc code mới và replay cả3 phản ví dụ cũ trong snapshot e7b ngoài workspace: O04b NULL nay FAIL đúng (16/1) và shell; duplicate+inject2 nay14/3/shell, không doublefree; pointermax+size13 nay15/2/shell, không đọcheader sai. Năm clean build CPUS1/formatter đạt: combined page12/0+object17/0, inject1/2 mỗi14/3, inject3/4 mỗi16/1 đúngexpected; tất cả shell. Ba lỗi cũ đã được sửa thực, khuyến nghị đánh giá mới thay nhận định chặnmerge của head0556. CI42467384992 SUCCESS.
- Evidence retest Khánh/Codex: C:/Users/KimSakota/.codex/tmp/BTL-W03-status-20261108/ (retest_khanh.py/results.json/build/log/replay). review_completed=true/format_exit0. Phạm vi này slab64; chưa class128/bản ghép W03/concurrent/usertests trong retest. Khánh đạt kỹ thuật slab64, còn Minh review và cập nhật hợp đồng biên/mixed64-128 khi N02 sẵn rồi ghép.
- Kết luận W03 chưa chốt: hai phần page và harness64 đạt kỹ thuật, N02 còn thiếu code sửa đã push; còn review thành viên, ghép và một log chung cùng revision (boot1/2/page-object-slab/usertests-q2), tổng hợp permalink/báo cáo5mục và bằng chứng nộp tại29. P05/fullCPU-matrix, S04/reclaim và client vẫn là W04, không giao lại trong W03. Không tính giờ/% thực tế từ quỹ dự kiến7–8giờ.
- Chỉ đọc GitHub/fetch e7b để kiểm tra bản sao và cập nhật bộ nhớ, không checkout/pull/stage/commit/push, sửa implementation hay đăng comment/approval/merge. Local vẫn codex/w03-page-stealing@79d997c/Csạch, bộ nhớ dirty.

## Review Khánh PR33 — ba lỗi harness cần sửa, 08/11/2026

- PR33 OPEN/MERGEABLE `codex/khanh-t03-object-tests` head `0556dd61327d0ff854aaef906900b8c49ba50851`, baseb276acd; CI42383386083 SUCCESS. Đã đọc5file đổi, Issue28/comment nhận việc ankhanh2594 và timelinePR (chưa có review/comment). Phạm vi T03 slab64 đúng: hook sau slab_test, init một lần, SLABTEST riêng/default không link. Có Refs28, kết quả/log/manifest/comment đầy đủ, class128 ghi phụ thuộcN02. Từ2107002 lên0556 chỉ đổi docs test matrix; raw log2107002 không bị coi mất giá trị vì source harness giữ nguyên blob020800526ad3a297e36ff6a22bd7f793eb4e497e. PR body còn câu chưa cập nhật matrix dù head đã cập nhật.
- Codex chạy nguyên snapshot0556 trong archive ngoài repo:6clean build CPUS1 default/object-normal/inject1/inject2/page-only/combined đều exit0 và tới shell. Normal15PASS0FAIL, inject1/2 mỗi12PASS3FAIL đúng ca báo cáo và cùngfree_after32725; page-only12PASS0FAIL; combined page12/0 vàobject15/0. Default không có symbol slabtest. Formatter3fileC/h đạt. Đây là kết quả Codex, không gán cho Khánh; chưa128/usertests/concurrent/integration W03.
- Finding P2 O04b tại slabtest.c245–250: kết quả chỉ xét free-page count, không bắt lỗi các lần cấp lại đã mất st_own. RSP giả lập chính xác kmalloc đầu trong O04b (call208 từ slabtest entry, size63) trảNULL bằng PC/a0 trước prologue, không thực hiện cấp trang/slot. Harness vẫn15PASS0FAIL/ALL_PASS và shell. Cần assertion đủ object cấp lại hợp lệ/duy nhất cùng điều kiện không thêm trang; thêm ca fault tái hiện.
- Finding P2 cleanup stash tại297–299: st_valid không bảo đảm ownership duy nhất hoặc chưafree. Trong inject2, RSP cho lần cấp thứ5 của bulk (call11 từ harness entry) trả lại object3 đang sống, không thực hiện lời gọi. st_corrupt cất alias vào stash1; st_release free object3 ở vòng st_own rồi free lại stash1. UART panic kmfree: double free detected, thiếuKTend/shell. Cần dedup stash với các object đã trả và giữa stash entries, vẫn chỉfree ownership hợp lệ. Không nói inject2 bình thường trên allocator đúng bị panic (baseline của nó đạt).
- Finding P2 kiểm tra địa chỉ tại80–83: p+size có thể trànuint64. RSP giả lập call9 (bulk object2,size13) trả0xfffffffffffffff8; cộngsizequấn về5 nên guard vượtqua và st_valid đọc header địa chỉ sai. Đã single-step tới loadfault chuyểnPC0 (stvec boot chưasetup), chưaKTend/shell. Cần chặn p ngoàiRAM trước tínhphầndư/kíchthước theo phép kiểmtra khôngtràn, trước đọc header. Không sửa source/instruction; các probe chỉ giả lập một return bằng debugger và quan sát.
- Evidence C:/Users/KimSakota/.codex/tmp/BTL-Khanh-PR33-review-20261108/ (review.py/results.json,6build/runtime logs,3probe logs). review_completed=true, ba counterexample lưu đủ; khuyến nghị sửa trướcmerge rồi Minh review assertion/cleanup. Chưa gửi comment/approval/merge hoặc sửa C/h/S; local Linh79d997c vẫn C sạch, chỉ bộ nhớ dirty. Đã đọc lạiPR cuối lượt vẫn0556/OPEN.

## Review Minh S03a — PR32 head988b60a, 08/11/2026

- Linh gửi nhánh feat(slab)-support-64-B-and-128-B-size-classes-and-md trên kimlinh25/BTL để đánh giá. Đã đọc Issue27, comment, compare main, source/design, PR32 và CI. PR32 đã tồn tại OPEN/MERGEABLE, head `988b60a7c1498a98ccd76c0c5a38a36715400517`, CI42380823443 SUCCESS. Nhánh chỉ đổi kernel/slab.c, không đổi docs dù tên có and-md. PR body vẫn template chưa điền, chưa có comment/review trong timeline đọc được; Issue27 mới có comment chốt thiết kế cũ.
- Code class chọn1..64/65..128 đúng,0/>128 trả0, layout32/capacity63/31, mỗi class một lock, free xác định class/owner. Codex chạy nguyên head trong archive ngoài workspace: bốn clean build CPUS1 normal và SLAB_TEST_CASE1/2/3 đều exit0. Normal S03a đủ4nhóm boundary/routing, LIFO, multi-slab/pattern hai class, OOM lấp62slot128/retry đều ALL PASSED và shell. Formatter kernel/slab.c đạt.
- Finding P2 chính sách fill bị mất: kmalloc tại163–164 release rồi return, không memset0 như S02; kmfree trước226 nối slot không memset1. Quan sát QEMU RSP chỉ breakpoint/read, không sửa code hoặc dữ liệu: kmalloc(1) trả slot64@87fff020 có next-pointer87fff060 và byte05 thay vì zero; sau kmfree, bytes8..63 vẫn05 thay vì01. Tài liệu đang hiệu lực docs/slab_allocator_design.md71/76 và implementationS02 yêu cầu0/1, nên là regression chưa được chốt đổi policy. Normal test hiện không bắt được lỗi này.
- Finding P2 test negative bị mất: đã build/boot cả SLAB_TEST_CASE1/2/3, tất cả ALL PASSED/tới shell thay vì panic; source đã xóa toàn bộ nhánh macro nhưng Makefile/docs vẫn quảng bá các chế độ này. Đây là mất ca test, không kết luận kmfree mất các guard double-free/invalid pointer (các guard vẫn có trong code).
- Tài liệu thiết kế cũng chưa cập nhật S03a: tiêu đề/mô tả vẫn một class64, size>64 bị từ chối, layoutmetadata cũ; cần cập nhật hai class trong cùng PR32 theo acceptance27. Bổ sung Refs27, revision/log và kết quả do chính Minh chạy vào body/comment; không gán log Codex thành Minh.
- Khuyến nghị chưa merge head hiện tại đến khi giữ lại0/1, phục hồi negative modes và đồng bộ thiết kế. Evidence/runner/raw logs/results ở C:/Users/KimSakota/.codex/tmp/BTL-Minh-S03a-review-20261108/. Source blob2344c6ca63fea8b99ee4df029ccb6607f7364d44; review_completed=true, normal expected đạt nhưng negative modes/fill contract không đạt. Chưa chạy usertests/bản ghép/concurrent slab trong review này; không tạo thêm điều kiện P05/S04/reclaim.
- Local của Linh đã79d997c feat(page): steal one page when local pool is empty và C sạch ở lần kiểm tra, chỉ bộ nhớ dirty. Không checkout/pull/stage/commit/push, không sửa C/h/S hoặc gửi comment/approval/merge thay; dùng git archive đúng SHA Minh để kiểm tra. PR32 đã được đọc lại cuối lượt, vẫn988b60a/OPEN.

## P04 đã qua kiểm tra kỹ thuật, chờ format/commit/PR — 08/11/2026

- Linh tự thêm if(r) break sau release donor, đúng vị trí. Review source không còn lỗi thiếu break; một khóa pool mỗi lần, guard ngoài cân bằng, fill5/return giữ chung. Local vẫn codex/w03-page-stealing@58ddd37, P04 chưa commit; proc.c chỉ dư khoảng trắng sau cpuid(). Search PR repo kimlinh25/BTL theo head này trả danh sách rỗng ở lần đọc.
- Codex build source snapshot HEAD58ddd37 cộng hai file đang sửa, không đổi implementation. SHA256 kalloc.c `c8b8b46e7d61facb9d0874f64e7f88fc3866ea13dcd138655e7e32881e6df0db`, proc.c `b5cfb5351fe4eac015371e7d655cbfb41a7635c2851f20d14e6b664e4098aebc`. Hai clean build CPUS1/2 KALLOCTEST1 exit0; cả hai page12PASS0FAIL/free32729 trước sau, slab64 ALL PASSED/shell. CPUS2 usertests -q kết thúc ALL TESTS PASSED (~79.67s cả boot/regression).
- Ca controlled local-miss CPUS2 thực tế bằng QEMU GDB RSP: dừng ở lần kalloc đầu sau kinit khi CPU khác còn chờ started, đọc CPU0/local0. Chuyển head list từ pool0 sang pool1 bằng ghi dữ liệu debugger khi toàn bộ hart dừng; không đổi C/instruction. Trước cấp local0 rỗng, donor1 headA=0x87fff000, nextB=0x87ffe000. Kalloc trả đúngA, donor head thànhB, local vẫn0; tám khóa đều nhả, noff0->0, fill5 đúng64byte đầu. Tiếp tục nguyên page harness12/0, count32729, slab/shell đạt. Đây là setup boot yên, không gọi là P05 stress/concurrent.
- Runner/evidence: C:/Users/KimSakota/.codex/tmp/BTL-P04-review-20261108/, gồm run_suite.py, results.json, working.diff, build-cpu1/2.log, normal-cpu1/2.log và controlled-local-miss.log. Suite exit0, source_still_matches=true. Người chạy Codex, raw revision58ddd37-dirty; không gán log cho Linh hoặc nhận là test clean commit P04 tương lai.
- Formatter/diff check còn trailing whitespace kalloc102/109 và proc65. Hướng dẫn Linh tự clang-format -i kernel/kalloc.c kernel/proc.c, kiểm tra proc trở về không diff; stage riêng kalloc.c, tự commit `feat(page): steal one page when local pool is empty`, push origin và mở PR P03/P04 Refs26 vào kimlinh25/BTL main. Minh review còn thiếu trước đóng26; bản ghép W03 chung vẫn là bước N04. Agent chỉ tạo runner ngoài repo/cập nhật bộ nhớ, không sửa C/h/S, stage/commit/push hoặc gửi comment/PR/approval/merge.

## Review P04 lần đầu — thiếu break, 08/11/2026

- Linh đã tự thêm nhánh local-miss sau release(local), trước pop_off trong kalloc. Quét donor bỏid, khóa donor/pop một head/release đúng. Thiếu điều kiện dừng khi lấy được trang: vòng sau ghi đè r, có thể làm mất trang đã detach và trả0 nếu donor cuối rỗng. Chưa đạt P04; chưa chạy kernel cho bản này.
- Giao sửa nhỏ: ngay sau release(&kmem[donor].lock) trong vòng for, nếu r!=0 thì break. Phải release trước break; giữ pop_off/fill5/return ngoài vòng lặp. Chờ Linh tự sửa rồi review lại.
- Có diff kernel/proc.c chỉ thêm khoảng trắng sau cpuid(); báo Linh xóa khoảng trắng thừa để giữ file đó nguyên trạng, không tự sửa/revert. kalloc.c cũng có trailing whitespace tại102/109. Agent chỉ cập nhật bộ nhớ, không sửa C/h/S hoặc commit/push.

## Linh đã tự test, commit/push P03 — 08/11/2026

- Linh gửi log normal CPUS1: page12PASS/0FAIL, slab64 ALL PASSED và shell; đã đọc lại ~/BTL-W03-P03-Linh.log trong Ubuntu, begin rev=b276acd-dirty/cpus1/inject0/free_before32729, end free_after32729. Log là của Linh, độc lập log review Codex. free pages after slab32726 phù hợp policy giữ cache, không lấy dòng sau slab làm expected bảo toàn của page harness.
- Local codex/w03-page-stealing tại commit `58ddd37faebcbdcea85b781b374f1ea190e76c6f` message feat(page): use CPU-local free lists, có upstream origin/codex/w03-page-stealing và tracking đang đồng bộ. Không có diff C. Linh tự thực hiện commit/push; agent không làm thay. Không gọi test trước commit thành test clean revision58ddd37: raw log ghi b276acd-dirty.
- kernel/kalloc.c hiện SHA256 `ab344b56fea53d1b55c141806ec7e8008361f5a88baba4094077dad003073af7`, đã được format; clang-format --dry-run --Werror không báo lỗi, git diff --check code đạt. Bộ nhớ agent vẫn dirty và giữ riêng khỏi code.
- Bước tự viết mới chỉ thêm nhánh P04 trong kalloc sau release(local), trước pop_off: nếu r==0 thì quét donor0..NCPU-1 bỏ id, khóa một donor, lấy/rút đúng một head nếu có, luôn release donor rồi mới break khi r!=0. Giữ guard ngoài, fill5/return hiện có. Đưa skeleton vòng lặp để Linh tự viết phần thân, không sửa C/h/S thay.
- P04 chưa được viết/chạy; controlled local-miss, build/boot CPUS2 và usertests -q còn thiếu trước chốt26. Không suy 1CPU/harness boot thành concurrency. Không đăng comment/review/merge hay commit/push thay.

## P03 local path đã kiểm tra một CPU — 08/11/2026

- Linh đã tự sửa cả kfree và kalloc trên codex/w03-page-stealing@b276acd, chưa commit code. Source review: guard trước cpuid, acquire/release đúng kmem[id], kalloc luôn pop_off sau release cả khi r==0, fill5 sau khi trang đã tách khỏi list. kfree giữ validation/fill1. Chưa có P04 stealing.
- Codex build bản sao HEAD cộng đúng kernel/kalloc.c hiện tại, không sửa implementation hoặc build trong workspace. SHA256 kalloc.c `80234fe78c88e360b608f8410d3c368833672579a1b1bc2c2671cd304b8227b1`. CPUS1/KALLOCTEST1/inject0 build exit0, page harness12PASS/0FAIL, free_before=free_after32729, slab64 ALL PASSED và shell. Bao gồm đường OOM trả0. Test runner cuối exit0, source snapshot và file gốc khớp SHA.
- Evidence/runner/build/raw log/results: C:/Users/KimSakota/.codex/tmp/BTL-P03-review-20261108-IMXdAG/. Lần đầu wrapper heredoc lỗi sau khi QEMU đã ghi kết quả và log /tmp không còn ở lượt sau; đã đổi sang runner file/durable evidence và chạy lại build/boot thành công. Không dùng kết quả wrapper lỗi làm bằng chứng duy nhất. Log là do Codex chạy, chưa phải log cá nhân Linh.
- clang-format21 --dry-run --Werror còn báo format tại56/65/66/78/90; git diff --check còn ba trailing whitespace56/66/78. Đây là việc dọn định dạng để đạt check, không phủ nhận kết quả logic/runtime P03. Hướng dẫn Linh tự chạy clang-format -i và test CPUS1 nếu cần log cá nhân, rồi tự stage riêng kalloc.c, commit `feat(page): use CPU-local free lists` và push origin kimlinh25/BTL.
- P03 local-only đã kiểm tra trong phạm vi trên, chưa đủ đóng Issue26. Bước tiếp là Linh tự viết P04 lấy một trang từ donor khi local rỗng, mỗi lần giữ một khóa; ca local-miss có kiểm soát và boot/usertests CPUS2 còn thiếu. Chưa usertests, nhiều CPU/concurrent hoặc PR approval; không tự stage/commit/push/sửa C/h/S.

## Review kfree của Linh — 08/11/2026

- Trên codex/w03-page-stealing, Linh đã tự sửa kfree: int id, push_off trước cpuid, đủ bốn tham chiếu kmem[id], release đúng khóa rồi pop_off; giữ validation và memset1 trước publish. Review source thấy đúng luồng cho lát cắt này. kalloc vẫn pool0, P03 chưa hoàn tất và chưa có stealing.
- git diff --check kernel/kalloc.c báo trailing whitespace tại56/66/78, không có lỗi logic từ các khoảng trắng này; dọn khi chốt code. Chú thích pop_off "Bật lại ngắt" cần hiểu là khôi phục trạng thái ngắt trước guard, không luôn bật. Chưa build/boot/test kernel ở lượt review.
- Bước đang giao: Linh tự sửa riêng kalloc local, thêm CPU ID và guard trước chọn pool, đổi đủ bốn tham chiếu pool, pop_off sau release và trước fill5/return để cả nhánh r==0 cũng cân bằng. Sau diff review mới chạy P03 CPUS1; P04 vẫn là việc tiếp theo. Không sửa C/h/S hoặc commit/push thay Linh.

## W03 — Linh đã tạo nhánh làm việc, kiểm tra 07/11/2026

- Local đang `codex/w03-page-stealing` tại `b276acd`, cùng revision với main/origin/main sau khi Linh tự cập nhật. Origin là https://github.com/kimlinh25/BTL.git; original giữ repo kimsakota. Nhánh W03 chưa có upstream ở lần đọc. Git status sạch trước cập nhật bộ nhớ này, không có diff C.
- Linh đã tự lưu tài liệu bằng commit `e6f0843` trên `codex/save-docs-20261107`. Bộ nhớ và kế hoạch W03 mới hơn đã cất ở nhánh đó; snapshot 31/10 bên dưới là lịch sử của main, không phản ánh tiến độ W03 hiện tại. Chưa đưa commit tài liệu vào nhánh code.
- Page harness W02 đã có trong local: kernel/kalloctest.c, KALLOCTEST trong Makefile và hook sau kinit. kfree/kalloc vẫn dùng pool0. Chưa có implementation P03/P04 hoặc kết quả kernel W03 mới của Linh.
- Tiếp tục bằng Visual Studio tím theo công cụ Linh đang dùng. Bài tự viết hiện tại chỉ kfree: khai báo CPU ID, guard trước cpuid, đổi đủ bốn tham chiếu pool0 trong hàm sang pool được chọn, khóa đúng pool, giữ validation/junk fill và cân bằng guard. Chờ Linh viết rồi đọc diff trước kalloc local CPUS1 và stealing một trang. Không sửa C/h/S, stage/commit/push, checkout hay chạy kernel thay trong lượt hướng dẫn.

Các mục 31/10 dưới đây được giữ để đối chiếu lịch sử.


Cập nhật: 31/10/2026.

## Quyết định vận hành W02 — 27/10/2026

- Linh chốt không tạo file/hồ sơ `docs/bao-cao/W02` riêng trong repo.
- Issue N01–N04 và các PR là nơi làm việc chính: mỗi buổi cập nhật comment có revision, lệnh, kết quả, link PR/log và blocker; review ghi ngay trên PR/Issue.
- Linh tổng hợp W02 bằng permalink comment/PR/log để chốt W03. Chỉ tạo file nộp ngoài GitHub nếu môn học bắt buộc, không dùng file đó làm sổ tiến độ.
- Comment nhận việc đầu tiên của Linh đã có trên Issue #17; chưa có comment W02 nào trên Issue #20.

## Project status

- Giai đoạn: GitHub Issues/Projects W02 #16–#20 và quy trình nhóm được giữ từ origin/main. Nhánh code `codex/slab-allocator_page` đã merge origin/main 13c39f3 bằng commit 3a9612c (tác giả Minhlth). WIP slab hiện vẫn local, chưa push; stash `W02 work before origin main sync` còn giữ bản dự phòng. Xem L033.
- Slab W02: single-class allocator 64B đã tích hợp init/test sau `kinit()`. QEMU in `slab_test: ALL PASSED`; `usertests`, `crash`, và ba panic cases đều đạt local ngày 31/10. Tài liệu thiết kế cập nhật. Chưa có per-CPU object freelists/client integration/benchmark.
- Mục tiêu: đề tài 19, Slab allocator and per-CPU free lists trên xv6; nối kiến thức từ BaoKhoaHoc/VstHelper.
- Người dùng trong chat này: Nguyễn Kim Linh; muốn làm nhiều và hiểu toàn bộ dự án.
- Nhóm dự án 26118: Nguyễn Kim Linh 20233495, Lê Trần Hiếu Minh 20233528, Nguyễn An Khánh 20224019. Linh xác nhận khoảng 10–12 giờ/người/tuần ngày 24/10; chưa xác nhận deadline chính thức và việc nhận toàn bộ phân công kỹ thuật của hai bạn.

## Stable facts

| ID | Statement | Confidence | Source | Revisit when | Last checked |
|---|---|---|---|---|---|
| CST-001 | Người dùng đã chốt đề tài 19 | Cao | LOGBOOK.md, L001; chat gốc | Người dùng đổi đề tài | 13/10/2026 |
| CST-002 | BTL có tài liệu, workspace và task cấu hình | Cao | README.md và các file hiện có | Cấu trúc dự án thay đổi | 13/10/2026 |
| CST-003 | Source xv6 phù hợp nằm ở thư mục Exercise được người dùng cung cấp | Cao | docs/Nguon-xv6.md | Import hoặc đổi baseline | 13/10/2026 |
| CST-004 | Baseline nguồn được kiểm tra là commit 06aad25c735fd3159bdfae5680be4eab7b1668b2 của mit-pdos/xv6-riscv | Cao | docs/Nguon-xv6.md | HEAD nguồn thay đổi | 13/10/2026 |
| CST-005 | Source đó dùng free list trang chung, PGSIZE=4096, NCPU=8; chưa có slab/stealing trong phần đã kiểm tra | Cao | docs/Nguon-xv6.md và kernel nguồn | Thay allocator hoặc phiên bản | 13/10/2026 |
| CST-006 | Nhánh slab hiện có một size class 64B, header 32B, 63 slot, freelist nhúng và một lock. `main()` gọi init/test sau `kinit()`. Boot test kiểm tra alignment, reuse, pattern, slab thứ hai và fault-injected OOM/retry. Chưa có per-CPU object freelists hoặc tích hợp client | Cao về source và kiểm thử local | `kernel/slab.c`, `kernel/main.c`; LOGBOOK L033 | Source/test mới hoặc thay đổi allocator | 31/10/2026 |
| CST-007 | Local có RISC-V GCC Linux, clang-format và QEMU. Ngày 31/10 đã clean-build, format, boot/QEMU, usertests và crash; xem L033 để biết lệnh và giới hạn | Cao về công cụ/kết quả trong lượt | L033 và lệnh chạy thực tế | Công cụ đổi hoặc trước build/run | 31/10/2026 |
| CST-008 | Bộ nhớ project-memory Light được tích hợp ở agent/, nối qua AGENTS.md | Cao | agent/README.md và các file bộ nhớ | Đổi bộ khung memory | 13/10/2026 |
| CST-009 | Git repo BTL có default branch origin/main và tag xv6-baseline local; tag chưa được push lên origin ở lần kiểm tra 19/10 | Cao | Git local, GitHub API ref và git ls-remote --tags origin; L016 | Push tag hoặc đổi nền source | 19/10/2026 |
| CST-010 | Mã dự án 26118, số đề tài 19; nhóm gồm Linh 20233495, Minh 20233528, Khánh 20224019; người dùng là Linh | Cao | Ảnh và yêu cầu trực tiếp 19/10; LOGBOOK L015 | Người dùng đính chính | 19/10/2026 |
| CST-011 | Run main39212699734 tại head53872b8 đã completed/success: linux format/build/usertests/crash success; macos build/job success. Run PR39210447048 tại head2df489d cũng success. Chưa có per-CPU/slab hoặc benchmark; không có kalloctest.c/kcsan.c MIT lab trong BTL | Cao tại lúc kiểm tra | GitHub run và jobs; L023 | Source/run mới | 19/10/2026 |
| CST-012 | Tuần 19–25/10/2026 mới khởi động nhóm; phần Linh đang học là học trước/chuẩn bị cá nhân, không tự tính là kết quả nhóm W01 | Cao | Người dùng trả lời trực tiếp ngày 19/10; LOGBOOK L015 | Người dùng đính chính hoặc có kết quả W01 thật | 19/10/2026 |
| CST-013 | Local update@c782bb5 có DOCX W01 và tài liệu đang sửa, chưa commit/push. origin/main439e89b sau fetch cũ. GitHub Project1 và Issues16–20 đã tạo thật, không cần commit source để chúng xuất hiện | Cao tại lúc kiểm tra | Git local; GraphQL/REST và public Project | Commit/merge hoặc đổi working tree/Project | 24/10/2026 |
| CST-014 | Settings main lần đọc 19/10 giữ như lịch sử, chưa tái audit. Ngày 24/10 xác nhận assignees repo gồm kimsakota, Minhlth, ankhanh2594; Project1 linked BTL/public, Minh/Khánh được cấp WRITER. Không coi đã gán Issue là đã nhận/hoàn thành | Cao về API đã đọc/ghi; Settings cũ chỉ có giá trị tại ngày đọc | L025/L031; GitHub API và Project1 | Cấu hình/quyền/thành viên đổi | 24/10/2026 |

## Active operating decisions

| ID | Operating decision | Why it is active now | Source | Revisit when | Last checked |
|---|---|---|---|---|---|
| OP-001 | Trao đổi bằng tiếng Việt, giải thích theo từng bước học | Người dùng đang học nền tảng | LOGBOOK.md L002 | Người dùng yêu cầu khác | 13/10/2026 |
| OP-002 | Giữ cấu trúc nhỏ, chia module C theo trách nhiệm trong xv6 | Người dùng yêu cầu không thừa thãi; khung hiện tại theo hướng này | docs/Kien-truc-du-an.md | Có nhu cầu module độc lập thực tế | 13/10/2026 |
| OP-003 | Tách sự thật đã kiểm chứng khỏi hướng thiết kế và giả thuyết | Đồ án chưa có implementation/benchmark | DOCS_GUIDE.md | Luôn áp dụng khi ghi kết quả | 13/10/2026 |
| OP-004 | Dùng tài liệu Light và snapshot local, không chạy memory service nền | Phù hợp yêu cầu thêm thư mục agent cho dự án nhỏ | LOGBOOK.md L006 | Cần hooks/tìm kiếm memory phức tạp | 13/10/2026 |
| OP-005 | Người dùng tự thiết kế/viết toàn bộ phần C; assistant dạy từng thành phần, giải thích reference và kiểm tra, không đưa lời giải hoàn chỉnh hoặc sửa implementation thay | Người dùng làm rõ mục tiêu hiểu và tự viết, không chỉ gõ theo code mẫu | LOGBOOK.md L008 | Người dùng yêu cầu rõ cách làm khác | 13/10/2026 |
| OP-006 | Khi chuyển bài, soạn Markdown đầy đủ kiến thức và chi tiết từng phần; thực hành chia chặng nhỏ, không giữ người dùng ở Lesson 0 khi họ đã xác nhận đọc xong | Người dùng yêu cầu tạo Lesson 1 đầy đủ, có nghiên cứu và source reference | LOGBOOK.md L011 | Người dùng đổi cách học | 14/10/2026 |
| OP-007 | Cuối phần code nêu diff/kiểm tra thật, gợi ý commit và nhắc người dùng tự commit/push; không tự stage/commit/push. Chú thích chỉ gợi ý khi cần, không bắt buộc và không trì hoãn mốc đã đạt để chỉnh trình bày comment | Linh giữ quy tắc tự commit/push, làm rõ không cần coi trọng chú thích | LOGBOOK L013, L019; AGENTS.md | Người dùng đổi quy tắc hoặc yêu cầu rõ thao tác Git | 19/10/2026 |
| OP-008 | Linh muốn tham gia sâu và hiểu toàn bộ, đồng thời chia việc độc lập cho đủ ba người. Owner theo module + review/giải thích chéo là phương án đề xuất, chưa được nhóm chốt | Yêu cầu tổ chức nhóm và báo cáo tuần | LOGBOOK.md L015; docs/Quy-trinh-nhom-26118.md | Nhóm xác nhận hoặc đổi vai trò | 19/10/2026 |
| OP-009 | Hướng dẫn Git bằng GitHub Desktop, chỉ rõ tên/vị trí nút; không mặc định giao lệnh Git terminal. Agent vẫn có thể đọc Git local để kiểm tra | Người dùng yêu cầu dùng GitHub Desktop và gửi ảnh UI | LOGBOOK L020; AGENTS.md | Người dùng đổi công cụ | 19/10/2026 |
| OP-010 | Ưu tiên hoàn thiện quy trình ba người: mời/accept, clone, chia sẻ lesson, nhận task/board và review. Không tự chuyển sang bài C khi Linh còn hỏi cách tổ chức GitHub | Linh đính chính assistant chuyển sang Lesson 3 quá sớm và yêu cầu quy trình thực hành chung | Yêu cầu trực tiếp; LOGBOOK L026; AGENTS.md | Nhóm vận hành được hoặc Linh yêu cầu chuyển bài | 19/10/2026 |

## Current context

| Area | Current value | Source | Revisit when | Last checked |
|---|---|---|---|---|
| Current priority | Issues + Projects đã triển khai: https://github.com/users/kimsakota/projects/1; tree view1, board view3, cha W02 #16, N01 Linh #17, N02 Minh #18, N03 Khánh #19, N04 cả ba #20. Trạng thái assignment cần kiểm tra lại trên GitHub trước khi đổi. WIP slab đang local sau merge main, chưa push/commit. | Project API history; source/Git L033 | Thành viên cập nhật hoặc publish code | 31/10/2026 |
| Build và runtime | CI baseline main39212699734 tại 53872b8 đạt format/build/usertests/crash và macOS build. Ngày 31/10 local clean-build, formatter, boot slab test, usertests, crash và ba panic cases đều đạt; chi tiết L033. Chưa có benchmark/per-CPU object freelist. | CI baseline L023; local L033 | Source/run mới hoặc test thêm | 31/10/2026 |
| Learning progress | Đã xác nhận đọc xong Lesson 0; Lesson 1–3 đã soạn. Hàm đếm đã vào main và CI baseline đạt; chưa suy ra Linh đã hoàn thành các bài thực hành khác | LOGBOOK L011–L014, L023; source và CI | Sau bài học tiếp theo | 19/10/2026 |
| Next implementation step | Linh có thể bắt đầu P02 sau baseline và review API ngắn: struct/mảng pool, init mọi khóa, ban đầu dùng pool0 và cập nhật count. P03 local-only là bước trung gian CPUS=1; P04/P05 hoàn thiện nhiều CPU ở W03. Không chờ học hết lesson và không tự viết C thay Linh | Lesson 3; kế hoạch W02; yêu cầu Linh 24/10 | Kết quả P02 hoặc nhóm đổi phạm vi | 24/10/2026 |
| Slab W02 implementation | Nhánh code hiện tại tích hợp `kmalloc_init()` và `slab_test()` sau `kinit()`; normal test (alignment, reuse, pattern, second slab, simulated OOM/retry), `usertests`, `crash`, double-free/misaligned/non-slab panic checks đều qua local 31/10. Merge `origin/main` 13c39f3 đã được ghi trong commit 3a9612c; giữ nội dung main/P02, không còn conflict. WIP/stash retained; chưa push. | LOGBOOK L033; source and local commands | Test/review or user publication | 31/10/2026 |

## Current constraints

- Source tham chiếu: `D:\2. FullStack\2026.1\Hệ điều hành\Excecise\4\xv6-riscv`.
- Source đó có sửa Makefile xử lý dấu cách và hai khác biệt mode script; cần ghi nhận khi tạo baseline.
- API page `kfree` của xv6 khác ý nghĩa object free trong đề tài; lộ trình đặt tên nằm trong tài liệu kiến trúc.
- Các mở rộng buddy, reclaim và adaptive stealing mới là hướng khảo sát, chưa có kết quả.

## Open questions

- Overview/rubric, giờ nộp chính thức, hạn cuối kỳ và ngày from/to thực tế của W01; tuần khởi động đã xác nhận 19–25/10/2026.
- Linh xác nhận cả ba đã xong onboarding; Minh có thiết kế S01 và tự báo cáo môi trường, Khánh vẫn ghi thiếu toolchain. Project/Issue URL đã có; còn cần các thành viên xác nhận nhận task, thu log cùng revision và permalink nộp báo cáo.
- Cách giảng viên định lượng sai khác 25% và cumulative progress; quy ước nội bộ trong tài liệu chỉ là đề xuất.
- Cách môn học tính ít nhất 3 cấu trúc kernel chuyển động và quy định tên API cuối.
- Hiệu quả của batch stealing/adaptive policy chỉ có thể kết luận sau thực nghiệm.
