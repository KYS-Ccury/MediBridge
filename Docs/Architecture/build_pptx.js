// MediBridge 시스템 아키텍처 PPTX 생성 — MVP 범위
// 실행: node build_pptx.js

const pptxgen = require("pptxgenjs");

const pres = new pptxgen();
pres.layout = "LAYOUT_WIDE";      // 13.3 × 7.5"
pres.author = "MediBridge Team";
pres.title  = "메디브릿지 시스템 아키텍처 (MVP)";

// ===== 색상 팔레트 — 메디컬 트러스트 톤 =====
const C = {
    bgDark:     "0B1F3A",   // 타이틀 슬라이드 어두운 남색
    bgLight:    "F4F7FB",   // 본문 슬라이드 배경
    primary:    "0F4C81",   // 주 색상 (딥 블루)
    secondary:  "1C7293",   // 보조 (틸)
    accent:     "F18F01",   // 강조 (의료 오렌지)
    text:       "1A2238",   // 본문 어두운 텍스트
    textMuted:  "5B6478",   // 약한 텍스트
    white:      "FFFFFF",
    line:       "D5DCE4",   // 구분선
    // 디바이스/통신 라벨용
    devClient:  "0F4C81",
    devServer:  "1C7293",
    devLLM:     "5A8C5A",   // 녹색 톤 (LLM)
    devVision:  "B85042",   // 적색 톤 (Vision)
    devStorage: "8B7355",   // 갈색 톤 (Storage)
    devPhone:   "3B5998",   // 파랑 (Phone)
    devExternal:"6B7280",   // 회색 (외부)
    arrowRest:  "1C7293",
    arrowUSB:   "F18F01",
    arrowHTTPS: "5A8C5A",
};

const FONT_H = "Malgun Gothic";
const FONT_B = "Malgun Gothic";

// =====================================================
// 헬퍼: 디바이스 박스 그리기
// =====================================================
function addDeviceBox(slide, opts) {
    const { x, y, w, h, title, subtitle, lines, color, titleColor } = opts;
    slide.addShape(pres.shapes.RECTANGLE, {
        x, y, w, h,
        fill: { color: C.white },
        line: { color: color, width: 2 },
        shadow: { type: "outer", color: "000000", blur: 8, offset: 2, angle: 90, opacity: 0.1 },
    });
    // 상단 컬러바
    slide.addShape(pres.shapes.RECTANGLE, {
        x, y, w, h: 0.25,
        fill: { color: color },
        line: { color: color, width: 0 },
    });
    // 제목
    slide.addText(title, {
        x: x + 0.1, y: y + 0.28, w: w - 0.2, h: 0.35,
        fontFace: FONT_H, fontSize: 14, bold: true,
        color: C.text, margin: 0, valign: "top",
    });
    if (subtitle) {
        slide.addText(subtitle, {
            x: x + 0.1, y: y + 0.62, w: w - 0.2, h: 0.25,
            fontFace: FONT_B, fontSize: 10,
            color: C.textMuted, margin: 0, valign: "top",
        });
    }
    if (lines && lines.length > 0) {
        const startY = subtitle ? y + 0.92 : y + 0.65;
        slide.addText(
            lines.map((l, i) => ({
                text: l,
                options: { breakLine: i < lines.length - 1 }
            })),
            {
                x: x + 0.12, y: startY, w: w - 0.2, h: h - (startY - y) - 0.1,
                fontFace: FONT_B, fontSize: 10,
                color: C.text, margin: 0, valign: "top",
                paraSpaceAfter: 2,
            }
        );
    }
}

// =====================================================
// 헬퍼: 화살표 + 라벨
// =====================================================
function addArrow(slide, x1, y1, x2, y2, opts = {}) {
    const color = opts.color || C.arrowRest;
    const width = opts.width || 2;
    const dashType = opts.dash ? "dash" : "solid";
    // PowerPoint LINE 은 bounding box 의 한쪽 corner → 대각 corner 로 그려짐.
    // 음수 w/h 면 PowerPoint(원본 앱)에서 위치/방향이 깨짐 → 정규화 + flipH/flipV.
    const flipH = x2 < x1;
    const flipV = y2 < y1;
    const x = Math.min(x1, x2);
    const y = Math.min(y1, y2);
    const w = Math.abs(x2 - x1);
    const h = Math.abs(y2 - y1);
    slide.addShape(pres.shapes.LINE, {
        x, y, w, h,
        flipH, flipV,
        line: {
            color, width, dashType,
            beginArrowType: opts.bidir ? "triangle" : "none",
            endArrowType: "triangle",
        },
    });
}

// 단일 라인 세그먼트 (양 끝 화살표 머리 옵션)
function addLineSeg(slide, x1, y1, x2, y2, opts = {}) {
    const color = opts.color || C.arrowRest;
    const width = opts.width || 2;
    const flipH = x2 < x1;
    const flipV = y2 < y1;
    const x = Math.min(x1, x2);
    const y = Math.min(y1, y2);
    const w = Math.abs(x2 - x1);
    const h = Math.abs(y2 - y1);
    slide.addShape(pres.shapes.LINE, {
        x, y, w, h, flipH, flipV,
        line: {
            color, width,
            dashType: opts.dash ? "dash" : "solid",
            beginArrowType: opts.beginArrow || "none",
            endArrowType:   opts.endArrow   || "none",
        },
    });
}

// 다중 꺾인 화살표 — points: [[x,y], [x,y], ...]
// 마지막 세그먼트에만 endArrow, opts.bidir 면 첫 세그먼트에 beginArrow.
function addElbowArrow(slide, points, opts = {}) {
    for (let i = 0; i < points.length - 1; i++) {
        const [x1, y1] = points[i];
        const [x2, y2] = points[i + 1];
        const isFirst = (i === 0);
        const isLast  = (i === points.length - 2);
        addLineSeg(slide, x1, y1, x2, y2, {
            color: opts.color, width: opts.width, dash: opts.dash,
            beginArrow: (isFirst && opts.bidir) ? "triangle" : "none",
            endArrow:   isLast ? "triangle" : "none",
        });
    }
}

function addArrowLabel(slide, x, y, w, text, color) {
    slide.addShape(pres.shapes.RECTANGLE, {
        x, y, w, h: 0.30,
        fill: { color: C.bgLight },                       // 슬라이드 배경과 동일 → 화살표가 자연스럽게 끊김
        line: { color: color || C.arrowRest, width: 1 },
    });
    slide.addText(text, {
        x: x + 0.03, y: y + 0.02, w: w - 0.06, h: 0.26,
        fontFace: FONT_B, fontSize: 9.5, bold: true,
        color: color || C.arrowRest,
        margin: 0, align: "center", valign: "middle",
    });
}

// =====================================================
// 헬퍼: 본문 슬라이드 헤더
// =====================================================
function addSlideHeader(slide, eyebrow, title) {
    slide.background = { color: C.bgLight };
    // 좌측 컬러바
    slide.addShape(pres.shapes.RECTANGLE, {
        x: 0, y: 0, w: 0.2, h: 7.5,
        fill: { color: C.primary }, line: { color: C.primary, width: 0 },
    });
    // 상단 작은 라벨
    slide.addText(eyebrow, {
        x: 0.5, y: 0.35, w: 12.3, h: 0.3,
        fontFace: FONT_H, fontSize: 11, bold: true,
        color: C.accent, charSpacing: 4, margin: 0,
    });
    // 제목
    slide.addText(title, {
        x: 0.5, y: 0.65, w: 12.3, h: 0.7,
        fontFace: FONT_H, fontSize: 28, bold: true,
        color: C.text, margin: 0,
    });
}

// =====================================================
// 슬라이드 1 — 타이틀
// =====================================================
{
    const s = pres.addSlide();
    s.background = { color: C.bgDark };

    // 좌측 액센트 굵은 줄
    s.addShape(pres.shapes.RECTANGLE, {
        x: 0, y: 0, w: 0.4, h: 7.5,
        fill: { color: C.accent }, line: { color: C.accent, width: 0 },
    });

    s.addText("MEDIBRIDGE", {
        x: 1.0, y: 1.6, w: 11, h: 0.5,
        fontFace: FONT_H, fontSize: 14, bold: true,
        color: C.accent, charSpacing: 12, margin: 0,
    });
    s.addText("시스템 아키텍처", {
        x: 1.0, y: 2.2, w: 11, h: 1.2,
        fontFace: FONT_H, fontSize: 56, bold: true,
        color: C.white, margin: 0,
    });
    s.addText("MVP 범위 — 5대 PC + 안드로이드 폰", {
        x: 1.0, y: 3.5, w: 11, h: 0.6,
        fontFace: FONT_H, fontSize: 22,
        color: "B6CDE0", margin: 0,
    });

    // 하단 정보 박스
    s.addShape(pres.shapes.RECTANGLE, {
        x: 1.0, y: 5.6, w: 11.3, h: 1.3,
        fill: { color: "1A3454" }, line: { color: "1A3454", width: 0 },
    });
    s.addText([
        { text: "복약·건강 통합 케어 플랫폼  ", options: { bold: true, color: C.white, fontSize: 14 } },
        { text: "—  알약 식별 · DUR 안전 점검 · 복약 이력 · 통합 보고서", options: { color: "B6CDE0", fontSize: 14 } },
    ], { x: 1.2, y: 5.7, w: 11.0, h: 0.4, fontFace: FONT_B, margin: 0, valign: "middle" });
    s.addText([
        { text: "팀: ", options: { bold: true, color: "B6CDE0", fontSize: 12 } },
        { text: "김윤식·심동주·인효     ", options: { color: C.white, fontSize: 12 } },
        { text: "담당 교수: ", options: { bold: true, color: "B6CDE0", fontSize: 12 } },
        { text: "이동녘     ", options: { color: C.white, fontSize: 12 } },
        { text: "개정: ", options: { bold: true, color: "B6CDE0", fontSize: 12 } },
        { text: "2026-05-08", options: { color: C.white, fontSize: 12 } },
    ], { x: 1.2, y: 6.2, w: 11.0, h: 0.4, fontFace: FONT_B, margin: 0, valign: "middle" });
}

// =====================================================
// 슬라이드 2 — 전체 아키텍처 다이어그램 (v5 — 허브 중심 + 직각 엘보 화살표)
// =====================================================
{
    const s = pres.addSlide();
    addSlideHeader(s, "ARCHITECTURE OVERVIEW", "전체 시스템 구성 — 5대 PC + 폰");

    // =====================================================
    // 레이아웃 — 격자 정렬 (모든 박스 동일 폭/높이, 직각 연결만)
    //
    //       col1(좌)        col2(중)        col3(우)
    //   ─────────────────────────────────────────────
    //   r1  안드S24                          식약처(외)
    //   r2  클라PC          [메인서버]       (빈칸)
    //   r3  LLM PC          Vision PC        데이터보관
    //
    //   메인서버 = 허브.  모든 화살표 직각(엘보) 또는 직선.
    // =====================================================
    const COL = { c1: 0.55,  c2: 3.40,  c3: 6.65 };   // 좌측 시작 x
    const ROW = { r1: 1.65,  r2: 3.60,  r3: 5.25 };   // 상단 시작 y (r3 위로 올려 하단 채널 공간 확보)
    const BW = 2.20, BH = 1.30;                        // 박스 폭/높이
    // 가로 채널 (메인 → 추론 부채꼴) y 좌표
    const ROUTE_MID = 5.07;       // bot(r2)=4.90 과 r3=5.25 사이
    // 데이터 평면 채널 (Client/Vision ↔ 보관 PC) — r3 박스 아래
    const ROUTE_DATA = 6.78;      // bot(r3)=6.55 와 legend 7.00 사이
    // 박스 중심 좌표 계산용
    const cx = (col) => col + BW / 2;
    const cy = (row) => row + BH / 2;

    // ----- compact 디바이스 박스 -----
    function box(col, row, label, sub, color) {
        const x = col, y = row;
        s.addShape(pres.shapes.RECTANGLE, {
            x, y, w: BW, h: BH,
            fill: { color: C.white },
            line: { color: color, width: 2 },
            shadow: { type: "outer", color: "000000", blur: 6, offset: 1.5, angle: 90, opacity: 0.12 },
        });
        s.addShape(pres.shapes.RECTANGLE, {
            x, y, w: BW, h: 0.16,
            fill: { color: color }, line: { color: color, width: 0 },
        });
        s.addText(label, {
            x: x + 0.10, y: y + 0.22, w: BW - 0.2, h: 0.32,
            fontFace: FONT_H, fontSize: 11.5, bold: true,
            color: C.text, margin: 0, valign: "top",
        });
        if (sub) {
            s.addText(sub, {
                x: x + 0.10, y: y + 0.55, w: BW - 0.2, h: BH - 0.6,
                fontFace: FONT_B, fontSize: 9,
                color: C.textMuted, margin: 0, valign: "top",
            });
        }
    }

    // ----- 번호 배지 -----
    function addBadge(x, y, num, color) {
        const r = 0.18;
        s.addShape(pres.shapes.OVAL, {
            x: x - r, y: y - r, w: r * 2, h: r * 2,
            fill: { color: color }, line: { color: C.white, width: 2 },
        });
        s.addText(num, {
            x: x - r, y: y - r, w: r * 2, h: r * 2,
            fontFace: FONT_H, fontSize: 11, bold: true,
            color: C.white, margin: 0, align: "center", valign: "middle",
        });
    }

    // =====================================================
    // 박스 배치 (격자 정렬)
    // =====================================================
    // r1 — 사용자 디바이스 + 외부
    box(COL.c1, ROW.r1, "안드로이드 S24",
        "Android 14+\n카메라 · 마이크 · STT", C.devPhone);
    box(COL.c3, ROW.r1, "식약처 OpenAPI (외부)",
        "apis.data.go.kr\n공공데이터포털", C.devExternal);

    // r2 — 클라PC + 메인서버 (허브)
    box(COL.c1, ROW.r2, "클라이언트 PC",
        "Windows 10/11\nQt6 + QML · TTS", C.devClient);
    box(COL.c2, ROW.r2, "메인 서버 PC",
        "10.10.10.97 : 8001\nDrogon C++ + MariaDB", C.devServer);

    // r3 — 추론 PC 들 + 데이터 보관
    box(COL.c1, ROW.r3, "LLM PC",
        "10.10.10.128 : 8002 · GPU\nStage 0.5 · RAG · ChromaDB", C.devLLM);
    box(COL.c2, ROW.r3, "Vision PC",
        "10.10.10.120 : 8003 · GPU\nYOLO · PaddleOCR · OpenCV", C.devVision);
    box(COL.c3, ROW.r3, "데이터 보관 PC",
        "10.10.10.122\n사진 전용 저장소", C.devStorage);

    // 각 박스의 모서리 좌표 헬퍼
    const right  = (col)       => col + BW;
    const bot    = (row)       => row + BH;
    const topMid = (col, row)  => [cx(col), row];
    const botMid = (col, row)  => [cx(col), bot(row)];
    const lftMid = (col, row)  => [col, cy(row)];
    const rgtMid = (col, row)  => [right(col), cy(row)];

    // =====================================================
    // 화살표 — 모두 직각 (수직/수평) 또는 엘보
    // =====================================================

    // ① 안드S24 ↔ 클라PC  (수직 직선, c1 column 가운데)
    addElbowArrow(s, [
        botMid(COL.c1, ROW.r1),       // (1.65, 2.95)
        topMid(COL.c1, ROW.r2),       // (1.65, 3.60)
    ], { color: C.arrowUSB, width: 2.5, bidir: true });
    addBadge(cx(COL.c1), (bot(ROW.r1) + ROW.r2) / 2, "1", C.arrowUSB);

    // ② 클라PC ↔ 메인서버  (수평 직선, r2 row 가운데)
    addElbowArrow(s, [
        rgtMid(COL.c1, ROW.r2),       // (2.75, 4.25)
        lftMid(COL.c2, ROW.r2),       // (3.40, 4.25)
    ], { color: C.arrowRest, width: 2.5, bidir: true });
    addBadge((right(COL.c1) + COL.c2) / 2, cy(ROW.r2), "2", C.arrowRest);

    // ⑦ 메인서버 ↔ 식약처  (엘보: 메인 위로 → 식약처 아래로)
    // 메인 top-mid (4.50, 3.60) → up to (4.50, 3.15) → right to (cx(c3)=7.75, 3.15) → up to (7.75, 식약처bot=2.95)
    // 더 단순: 메인 top-mid 위쪽 (4.50, 3.30) → 수평 → 식약처 bot-mid 위쪽 (7.75, 3.30) → 수직위 → (7.75, 식약처 bottom)
    addElbowArrow(s, [
        topMid(COL.c2, ROW.r2),                       // (4.50, 3.60)
        [cx(COL.c2), 3.25],                            // up
        [cx(COL.c3), 3.25],                            // right
        botMid(COL.c3, ROW.r1),                        // (7.75, 2.95) — 식약처 하단 도착
    ], { color: C.arrowHTTPS, width: 2.5, bidir: true });
    addBadge((cx(COL.c2) + cx(COL.c3)) / 2, 3.25, "7", C.arrowHTTPS);

    // ④ 메인서버 ↔ Vision  (수직 직선, c2 column 가운데)
    addElbowArrow(s, [
        botMid(COL.c2, ROW.r2),       // (4.50, 4.90)
        topMid(COL.c2, ROW.r3),       // (4.50, 5.55)
    ], { color: C.arrowRest, width: 2.5, bidir: true });
    addBadge(cx(COL.c2), (bot(ROW.r2) + ROW.r3) / 2, "4", C.arrowRest);

    // ③ 메인서버 ↔ LLM  (엘보: 메인 좌하단 → 가로채널 → LLM 상단)
    addElbowArrow(s, [
        [COL.c2 + 0.40, bot(ROW.r2)], // (3.80, 4.90)
        [COL.c2 + 0.40, ROUTE_MID],
        [cx(COL.c1),    ROUTE_MID],
        topMid(COL.c1, ROW.r3),
    ], { color: C.arrowRest, width: 2.2, bidir: true });
    addBadge((COL.c2 + 0.40 + cx(COL.c1)) / 2, ROUTE_MID, "3", C.arrowRest);

    // ⑤ 메인서버 ↔ 데이터보관  (엘보: 메인 우하단 → 가로채널 → 데이터 상단)
    addElbowArrow(s, [
        [right(COL.c2) - 0.40, bot(ROW.r2)], // (5.20, 4.90)
        [right(COL.c2) - 0.40, ROUTE_MID],
        [cx(COL.c3),           ROUTE_MID],
        topMid(COL.c3, ROW.r3),
    ], { color: C.arrowRest, width: 2.2, bidir: true });
    addBadge((right(COL.c2) - 0.40 + cx(COL.c3)) / 2, ROUTE_MID, "5", C.arrowRest);

    // ⑥-a Vision ↔ 데이터보관  (수평 직선, r3 row 내, dashed)
    addElbowArrow(s, [
        rgtMid(COL.c2, ROW.r3),       // (5.60, 5.90)
        lftMid(COL.c3, ROW.r3),       // (6.65, 5.90)
    ], { color: C.arrowHTTPS, width: 2.2, dash: true, bidir: true });
    addBadge((right(COL.c2) + COL.c3) / 2, cy(ROW.r3), "6", C.arrowHTTPS);

    // ⑥-b 클라PC ↔ 데이터보관  (데이터 평면 직접 업로드 — c1-c2 갭 + 하단 채널, dashed)
    // 메인 발급 토큰으로 메인 우회 직접 PUT/GET. ⑥ 와 동일 패턴.
    // 클라PC 우측 → c1-c2 갭(x=3.075) → 하단 채널(y=ROUTE_DATA) → Storage 하단
    // (LLM 박스가 c1 r3 에 있어 직강하 불가 → 갭 경유 필수)
    const GAP_C1C2 = (right(COL.c1) + COL.c2) / 2;   // 3.075
    addElbowArrow(s, [
        [right(COL.c1), 4.55],         // (2.75, 4.55) — Client 우측, 화살표 ②(y=4.25) 아래
        [GAP_C1C2, 4.55],
        [GAP_C1C2, ROUTE_DATA],
        [cx(COL.c3), ROUTE_DATA],
        botMid(COL.c3, ROW.r3),         // (7.75, bot(r3)) — Storage 하단 중앙
    ], { color: C.arrowHTTPS, width: 2.2, dash: true, bidir: true });
    addBadge((GAP_C1C2 + cx(COL.c3)) / 2, ROUTE_DATA, "6", C.arrowHTTPS);

    // =====================================================
    // 우측 사이드 패널 — 통신 번호 가이드
    // =====================================================
    const SP_X = 9.10, SP_W = 4.05;
    s.addShape(pres.shapes.RECTANGLE, {
        x: SP_X, y: 1.55, w: SP_W, h: 5.30,
        fill: { color: C.white }, line: { color: C.line, width: 1 },
    });
    s.addText("통신 번호 가이드", {
        x: SP_X + 0.15, y: 1.65, w: SP_W - 0.3, h: 0.35,
        fontFace: FONT_H, fontSize: 14, bold: true,
        color: C.primary, margin: 0,
    });

    const commGuide = [
        ["1", "USB · ADB",        "폰 ↔ 클라PC",              C.arrowUSB],
        ["2", "REST · 8001",      "클라PC ↔ 메인",            C.arrowRest],
        ["3", "REST · 8002",      "메인 ↔ LLM PC",            C.arrowRest],
        ["4", "REST · 8003",      "메인 ↔ Vision PC",         C.arrowRest],
        ["5", "컨트롤 플레인",    "메인 ↔ 데이터 보관 PC",    C.arrowRest],
        ["6", "PUT / GET (직접)", "클라 · Vision ↔ 보관 PC",  C.arrowHTTPS],
        ["7", "HTTPS · 443",      "메인 ↔ 식약처 (외부)",     C.arrowHTTPS],
    ];

    const guideY0 = 2.15;
    const rowH = 0.60;
    commGuide.forEach(([num, label, route, color], i) => {
        const y = guideY0 + i * rowH;
        s.addShape(pres.shapes.OVAL, {
            x: SP_X + 0.20, y: y, w: 0.36, h: 0.36,
            fill: { color: color }, line: { color: color, width: 0 },
        });
        s.addText(num, {
            x: SP_X + 0.20, y: y, w: 0.36, h: 0.36,
            fontFace: FONT_H, fontSize: 13, bold: true,
            color: C.white, margin: 0, align: "center", valign: "middle",
        });
        s.addText([
            { text: label, options: { bold: true, color: C.text, fontSize: 11, breakLine: true } },
            { text: route, options: { color: C.textMuted, fontSize: 10 } },
        ], {
            x: SP_X + 0.65, y: y - 0.02, w: SP_W - 0.75, h: 0.46,
            fontFace: FONT_B, margin: 0, valign: "middle",
        });
    });

    // =====================================================
    // 하단 범례 (y 7.05~ 으로 내림 — 데이터 평면 채널 공간 확보)
    // =====================================================
    s.addShape(pres.shapes.RECTANGLE, {
        x: 0.4, y: 7.05, w: 12.7, h: 0.40,
        fill: { color: C.white }, line: { color: C.line, width: 1 },
    });
    s.addText([
        { text: "범례  ", options: { bold: true, fontSize: 10, color: C.textMuted } },
        { text: "━ ", options: { bold: true, fontSize: 14, color: C.arrowRest } },
        { text: "REST/HTTP (JSON)    ", options: { fontSize: 10, color: C.text } },
        { text: "━ ", options: { bold: true, fontSize: 14, color: C.arrowUSB } },
        { text: "USB (ADB)    ", options: { fontSize: 10, color: C.text } },
        { text: "‐ ‐ ‐ ", options: { bold: true, fontSize: 14, color: C.arrowHTTPS } },
        { text: "데이터 평면 (PUT/GET 직접)    ", options: { fontSize: 10, color: C.text } },
        { text: "━ ", options: { bold: true, fontSize: 14, color: C.arrowHTTPS } },
        { text: "HTTPS (외부)    ", options: { fontSize: 10, color: C.text } },
        { text: "①~⑦ ", options: { bold: true, fontSize: 10, color: C.primary } },
        { text: "통신 구간 — 우측 가이드 참조", options: { fontSize: 10, color: C.text } },
    ], {
        x: 0.6, y: 7.07, w: 12.5, h: 0.36,
        fontFace: FONT_B, margin: 0, valign: "middle",
    });
}

// =====================================================
// 슬라이드 3 — 디바이스/포트/기술 매트릭스
// =====================================================
{
    const s = pres.addSlide();
    addSlideHeader(s, "DEVICE MATRIX", "디바이스 · 포트 · 기술 스택");

    const headerRow = [
        { text: "#",       options: { bold: true, color: C.white, fill: { color: C.primary }, align: "center" } },
        { text: "디바이스", options: { bold: true, color: C.white, fill: { color: C.primary } } },
        { text: "IP / 호스트", options: { bold: true, color: C.white, fill: { color: C.primary } } },
        { text: "포트",     options: { bold: true, color: C.white, fill: { color: C.primary }, align: "center" } },
        { text: "OS / HW", options: { bold: true, color: C.white, fill: { color: C.primary } } },
        { text: "기술 스택", options: { bold: true, color: C.white, fill: { color: C.primary } } },
        { text: "주요 역할", options: { bold: true, color: C.white, fill: { color: C.primary } } },
    ];
    const rows = [
        ["①", "안드로이드 S24",        "USB / adb",          "—",     "Android 14+", "Galaxy AI · adb",                      "카메라 · 마이크 · 온디바이스 STT"],
        ["②", "클라이언트 PC",          "localhost",          "—",     "Windows 10/11", "Qt6 + C++ + QML · QTextToSpeech",     "사용자 GUI · 폰 화면 캡쳐 · TTS"],
        ["③", "메인 서버 PC",           "10.10.10.97",        "8001",  "Ubuntu 24.04",  "Drogon C++ + MariaDB",                 "비즈니스 로직 · 가명 매핑 · DUR"],
        ["④", "LLM 학습·추론 PC",       "10.10.10.128",       "8002",  "Ubuntu 24.04 + GPU", "FastAPI(예정) · ChromaDB · LLM", "Stage 0.5 · Onboarding RAG · 일반 안내"],
        ["⑤", "Vision 학습·추론 PC",    "10.10.10.120",       "8003",  "Ubuntu 24.04 + GPU", "FastAPI(예정) · YOLO · PaddleOCR · OpenCV", "알약 검출 · 각인 · 색·모양"],
        ["⑥", "데이터 보관 PC",         "10.10.10.122",       "8004",  "Ubuntu 24.04 + 대용량 디스크", "Drogon C++ 미니 서버",         "PUT/GET 직접 (HMAC 토큰 검증)"],
        ["—", "식약처 OpenAPI (외부)",  "apis.data.go.kr",    "443",   "외부 공공데이터포털", "HTTPS · OpenAPI",                  "낱알식별 / DUR / e약은요"],
    ];

    const tableData = [headerRow, ...rows.map(r => r.map((c, i) => ({
        text: String(c),
        options: {
            fontSize: 11, fontFace: FONT_B,
            color: C.text,
            valign: "middle",
            align: (i === 0 || i === 3) ? "center" : "left",
        }
    })))];

    s.addTable(tableData, {
        x: 0.5, y: 1.6, w: 12.3, h: 5.0,
        colW: [0.5, 2.0, 1.7, 0.9, 1.9, 2.7, 2.6],
        rowH: 0.55,
        border: { type: "solid", pt: 1, color: C.line },
        fontFace: FONT_B, fontSize: 11,
    });

    // 하단 안내
    s.addText("NOTE   ④·⑤는 학습+추론 동거 (여유 PC 부족) — 포트·네트워크 정책상 향후 PC 증설 시 즉시 분리 가능", {
        x: 0.5, y: 6.85, w: 12.3, h: 0.5,
        fontFace: FONT_B, fontSize: 11, italic: true,
        color: C.textMuted, margin: 0, valign: "middle",
    });
}

// =====================================================
// 헬퍼: 통신 슬라이드 공통 레이아웃
// =====================================================
function addCommSlide(opts) {
    const s = pres.addSlide();
    addSlideHeader(s, opts.eyebrow, opts.title);

    // 좌측: 송수신 다이어그램
    const leftX = 0.5, leftW = 5.0;
    s.addShape(pres.shapes.RECTANGLE, {
        x: leftX, y: 1.6, w: leftW, h: 5.5,
        fill: { color: C.white }, line: { color: C.line, width: 1 },
    });
    s.addText("연결 다이어그램", {
        x: leftX + 0.15, y: 1.7, w: leftW - 0.3, h: 0.35,
        fontFace: FONT_H, fontSize: 13, bold: true,
        color: C.textMuted, margin: 0,
    });

    // 양쪽 디바이스 박스 (큰 카드 형태)
    const cardY = 2.2, cardH = 1.5;
    const cardW = (leftW - 0.6) / 2;
    // A 카드
    s.addShape(pres.shapes.RECTANGLE, {
        x: leftX + 0.2, y: cardY, w: cardW, h: cardH,
        fill: { color: opts.colorA }, line: { color: opts.colorA, width: 0 },
    });
    s.addText(opts.deviceA, {
        x: leftX + 0.25, y: cardY + 0.1, w: cardW - 0.1, h: cardH - 0.2,
        fontFace: FONT_H, fontSize: 13, bold: true, color: C.white,
        margin: 0, align: "center", valign: "middle",
    });
    // B 카드
    s.addShape(pres.shapes.RECTANGLE, {
        x: leftX + 0.4 + cardW, y: cardY, w: cardW, h: cardH,
        fill: { color: opts.colorB }, line: { color: opts.colorB, width: 0 },
    });
    s.addText(opts.deviceB, {
        x: leftX + 0.45 + cardW, y: cardY + 0.1, w: cardW - 0.1, h: cardH - 0.2,
        fontFace: FONT_H, fontSize: 13, bold: true, color: C.white,
        margin: 0, align: "center", valign: "middle",
    });
    // 화살표
    const arrowY = cardY + cardH / 2;
    addArrow(s, leftX + 0.2 + cardW, arrowY, leftX + 0.4 + cardW, arrowY, {
        color: opts.arrowColor || C.arrowRest, width: 3, bidir: true,
    });

    // 통신 정보 박스
    const infoY = cardY + cardH + 0.35;
    const infoH = 3.2;
    s.addShape(pres.shapes.RECTANGLE, {
        x: leftX + 0.2, y: infoY, w: leftW - 0.4, h: infoH,
        fill: { color: C.bgLight }, line: { color: C.line, width: 1 },
    });
    s.addText([
        { text: "프로토콜",  options: { bold: true, color: C.textMuted, fontSize: 11, breakLine: true } },
        { text: opts.protocol, options: { color: C.text, fontSize: 12, breakLine: true } },
        { text: " ", options: { fontSize: 6, breakLine: true } },
        { text: "포트",       options: { bold: true, color: C.textMuted, fontSize: 11, breakLine: true } },
        { text: opts.port, options: { color: C.text, fontSize: 12, breakLine: true } },
        { text: " ", options: { fontSize: 6, breakLine: true } },
        { text: "포맷",       options: { bold: true, color: C.textMuted, fontSize: 11, breakLine: true } },
        { text: opts.format, options: { color: C.text, fontSize: 12, breakLine: true } },
        { text: " ", options: { fontSize: 6, breakLine: true } },
        { text: "방향",       options: { bold: true, color: C.textMuted, fontSize: 11, breakLine: true } },
        { text: opts.direction, options: { color: C.text, fontSize: 12 } },
    ], {
        x: leftX + 0.35, y: infoY + 0.15, w: leftW - 0.7, h: infoH - 0.25,
        fontFace: FONT_B, margin: 0, valign: "top",
        paraSpaceAfter: 2,
    });

    // 우측: 데이터·시나리오
    const rightX = 6.0, rightW = 6.8;
    s.addText("주고받는 데이터", {
        x: rightX, y: 1.6, w: rightW, h: 0.4,
        fontFace: FONT_H, fontSize: 14, bold: true,
        color: C.primary, margin: 0,
    });
    s.addText(
        opts.dataItems.map((it, i) => ({
            text: it,
            options: { bullet: true, breakLine: i < opts.dataItems.length - 1 }
        })),
        {
            x: rightX, y: 2.0, w: rightW, h: 1.8,
            fontFace: FONT_B, fontSize: 12, color: C.text,
            margin: 0, paraSpaceAfter: 4, valign: "top",
        }
    );

    s.addText("주요 시나리오", {
        x: rightX, y: 4.0, w: rightW, h: 0.4,
        fontFace: FONT_H, fontSize: 14, bold: true,
        color: C.primary, margin: 0,
    });
    s.addText(
        opts.scenarios.map((it, i) => ({
            text: it,
            options: { bullet: true, breakLine: i < opts.scenarios.length - 1 }
        })),
        {
            x: rightX, y: 4.4, w: rightW, h: 1.8,
            fontFace: FONT_B, fontSize: 12, color: C.text,
            margin: 0, paraSpaceAfter: 4, valign: "top",
        }
    );

    if (opts.note) {
        s.addShape(pres.shapes.RECTANGLE, {
            x: rightX, y: 6.4, w: rightW, h: 0.8,
            fill: { color: "FFF6E5" }, line: { color: C.accent, width: 1 },
        });
        s.addText("NOTE   " + opts.note, {
            x: rightX + 0.15, y: 6.45, w: rightW - 0.3, h: 0.7,
            fontFace: FONT_B, fontSize: 11, italic: true,
            color: C.text, margin: 0, valign: "middle",
        });
    }
}

// =====================================================
// 슬라이드 4 — 통신 ① 폰 ↔ 클라PC
// =====================================================
addCommSlide({
    eyebrow: "COMMUNICATION ①",
    title: "안드로이드 S24 ↔ 클라이언트 PC",
    deviceA: "안드로이드 S24",
    deviceB: "클라이언트 PC",
    colorA: C.devPhone, colorB: C.devClient,
    arrowColor: C.arrowUSB,
    protocol: "USB · Android Debug Bridge (ADB)",
    port: "adb 데몬 5037 (PC측 로컬)",
    format: "PNG 바이너리 (사진), UTF-8 텍스트 (STT 결과)",
    direction: "양방향 — 클라 → 폰 (촬영 트리거 / TTS 명령) / 폰 → 클라 (사진 · STT 텍스트)",
    dataItems: [
        "촬영 명령 (`adb exec-out screencap -p`) — 폰 카메라 화면 캡쳐 → PNG 스트림",
        "온디바이스 STT 결과 — 폰의 Galaxy AI / SpeechRecognizer 가 변환한 한국어 텍스트",
        "(선택) 폰 TTS 출력 — 음성 안내를 폰 스피커로 위임",
    ],
    scenarios: [
        "사용자가 클라 GUI 의 '촬영 + 식별' 버튼 클릭 → adb 가 폰 카메라 화면 캡쳐 회수",
        "음성 등록 모드 — 폰이 STT 변환 후 텍스트만 송신 (음성 바이너리 비송신)",
        "능동/대화형 가이드 — TTS 안내를 폰 스피커 또는 클라 PC 스피커로 출력",
    ],
    note: "음성 바이너리는 폰 외부로 나가지 않음 → 대역폭 절감 + 개인정보 노출 위험 ↓",
});

// =====================================================
// 슬라이드 5 — 통신 ② 클라PC ↔ 메인서버
// =====================================================
addCommSlide({
    eyebrow: "COMMUNICATION ②",
    title: "클라이언트 PC ↔ 메인 서버",
    deviceA: "클라이언트 PC",
    deviceB: "메인 서버 PC\n10.10.10.97 : 8001",
    colorA: C.devClient, colorB: C.devServer,
    arrowColor: C.arrowRest,
    protocol: "REST API on HTTP (Drogon C++)",
    port: "TCP 8001",
    format: "JSON (인증/약풀/이력/보고서/사진 메타) — 본문 multipart 없음 (사진은 ⑥ 직접 PUT)",
    direction: "양방향 — 클라가 요청, 메인 서버가 응답. JWT(HS256) 인증",
    dataItems: [
        "Auth — /v1/auth/{signup, login, logout}  (JWT)",
        "Media — /v1/media/{intent, commit, get_token}  [NEW]  사진 토큰 발급",
        "Pill — /v1/pill/{identify, identify/narrow, onboarding/normalize}",
        "Pool — /v1/pill/pool[/{id}|/all]  (GET/POST/DELETE)",
        "History — /v1/history/{record, list}",
        "Report — /v1/report/generate (JSON / HTML / PDF)",
        "Speech — /v1/speech/utterance (STT 의도 분류)",
    ],
    scenarios: [
        "로그인 → JWT 받음 → 이후 모든 요청에 Authorization: Bearer 헤더",
        "촬영 + 식별 — ① /media/intent (메타·토큰) → ② 클라가 보관 PC 직접 PUT(⑥) → ③ /media/commit → ④ /pill/identify",
        "음성 등록 → /v1/speech/utterance (의도 분류) → /v1/pill/onboarding/normalize (다회 round-trip)",
    ],
    note: "사진 본체는 메인서버 통과 X — 메인은 토큰만 발급(⑤), 본체는 ⑥ 데이터 평면 직접 PUT/GET",
});

// =====================================================
// 슬라이드 6 — 통신 ③ 메인서버 ↔ LLM PC
// =====================================================
addCommSlide({
    eyebrow: "COMMUNICATION ③",
    title: "메인 서버 ↔ LLM 학습·추론 PC",
    deviceA: "메인 서버 PC\n10.10.10.97",
    deviceB: "LLM PC\n10.10.10.128 : 8002",
    colorA: C.devServer, colorB: C.devLLM,
    arrowColor: C.arrowRest,
    protocol: "REST API on HTTP (FastAPI 예정)",
    port: "TCP 8002",
    format: "JSON 요청·응답 (텍스트 입력 위주)",
    direction: "메인서버 → LLM PC 단방향 요청 / LLM PC → 메인서버 응답",
    dataItems: [
        "Stage 0.5 의도 분류 — 사용자 발화 텍스트 → 카테고리 (식별/등록/이력 등)",
        "Onboarding RAG — 약명 정규화 + ChromaDB 벡터 검색 + 동명·동성분 분기 질문",
        "비의료 일반 안내 보조 — e약은요 비위험 정보 자연어 요약",
        "(미사용) 의료 안내 영역 — LLM·RAG 절대 미사용 (DUR 위험 안내는 정해진 템플릿)",
    ],
    scenarios: [
        "사용자 '타이레놀 등록할게' → 메인이 LLM PC 에 NER + RAG 요청 → 후보 N개 반환",
        "후보 2~5건 → 분기 질문 생성 → 클라 TTS 안내 → 사용자 응답 round-trip",
        "단일 후보 → RESOLVED → 메인이 /v1/pill/pool 로 등록 진행",
    ],
    note: "학습+추론 동거 (네트워크 분리 가능 설계). ChromaDB 벡터 DB 동거 (~수십 GB 임베딩)",
});

// =====================================================
// 슬라이드 7 — 통신 ④ 메인서버 ↔ Vision PC
// =====================================================
addCommSlide({
    eyebrow: "COMMUNICATION ④",
    title: "메인 서버 ↔ Vision 학습·추론 PC",
    deviceA: "메인 서버 PC\n10.10.10.97",
    deviceB: "Vision PC\n10.10.10.120 : 8003",
    colorA: C.devServer, colorB: C.devVision,
    arrowColor: C.arrowRest,
    protocol: "REST API on HTTP (FastAPI 예정)",
    port: "TCP 8003",
    format: "JSON 요청 (storage_url + get_token 포함), JSON 응답 (검출 결과)",
    direction: "메인서버 → Vision PC 추론 요청 / Vision PC → 데이터 보관 PC 사진 직접 GET (⑥)",
    dataItems: [
        "POST /vision/detect_remote — 메인이 보내는 페이로드:",
        "  { photo_id, storage_url, get_token(HS256 op=get), mime, purpose }",
        "Vision PC 동작 — get_token 으로 보관 PC 에 직접 GET → YOLO · PaddleOCR · OpenCV 추론",
        "Narrow Down — 사용자 속성(색/모양/각인) 입력 받아 후보 좁히기 (Rule-based, LLM 미사용)",
        "응답 schema — { candidates:[{item_code,drug_name,confidence,match_keys}], confidence_tier }",
    ],
    scenarios: [
        "클라가 사진 업로드 → 메인이 /v1/pill/identify → Vision PC 로 라우팅 → 결과를 클라에 반환",
        "신뢰도 낮을 때 → /v1/pill/identify/narrow → 사용자 추가 속성 입력 → 후보 좁힘",
        "학습 시점 — Vision PC 가 데이터 보관 PC 에서 학습 데이터 일괄 PULL (통신 ⑥ 참조)",
    ],
    note: "현재 PaddleOCR 사전학습 + OpenCV 휴리스틱. AI Hub 데이터 fine-tuning 으로 정확도 향상 예정",
});

// =====================================================
// 슬라이드 8 — 통신 ⑤·⑥ 데이터 보관 PC (컨트롤·데이터 평면 분리)
// =====================================================
{
    const s = pres.addSlide();
    addSlideHeader(s, "COMMUNICATION ⑤·⑥", "데이터 보관 PC — 컨트롤 / 데이터 평면 분리");

    // 좌측 — 컨트롤 평면
    s.addShape(pres.shapes.RECTANGLE, {
        x: 0.5, y: 1.7, w: 6.0, h: 5.5,
        fill: { color: C.white }, line: { color: C.devServer, width: 2 },
    });
    s.addText([
        { text: "⑤ 컨트롤 평면", options: { bold: true, fontSize: 18, color: C.devServer, breakLine: true } },
        { text: "메인 서버 ↔ 데이터 보관 PC", options: { fontSize: 12, color: C.textMuted } },
    ], {
        x: 0.7, y: 1.85, w: 5.6, h: 0.7,
        fontFace: FONT_H, margin: 0, valign: "top",
    });
    s.addText([
        { text: "프로토콜 ", options: { bold: true, color: C.textMuted, fontSize: 11, breakLine: false } },
        { text: "메인 REST · 8001 (Drogon C++)", options: { color: C.text, fontSize: 11, breakLine: true } },
        { text: "역할 ", options: { bold: true, color: C.textMuted, fontSize: 11, breakLine: false } },
        { text: "사진 메타 관리 · 단기 HMAC 토큰 발급 · 보관 PC /health 폴링", options: { color: C.text, fontSize: 11, breakLine: true } },
        { text: "포맷 ", options: { bold: true, color: C.textMuted, fontSize: 11, breakLine: false } },
        { text: "JSON (request_id·photo_id·storage_url·put_token·get_token·expires_at)", options: { color: C.text, fontSize: 11, breakLine: true } },
    ], {
        x: 0.7, y: 2.8, w: 5.6, h: 1.5,
        fontFace: FONT_B, margin: 0, valign: "top", paraSpaceAfter: 6,
    });
    s.addText("메인서버 엔드포인트 (구현 완료)", {
        x: 0.7, y: 4.35, w: 5.6, h: 0.3,
        fontFace: FONT_H, fontSize: 12, bold: true, color: C.primary, margin: 0,
    });
    s.addText(
        [
            "POST /v1/media/intent — photo_id·storage_url·put_token (TTL 300s)",
            "PUT 보관 PC 후 POST /v1/media/commit — PENDING → READY",
            "POST /v1/media/get_token — Vision PC 용 GET 토큰 (op=get)",
            "photo_storage 메타 INSERT/UPDATE (anon·path·mime·status·expires_at)",
            "메인 ↔ 보관 PC GET /health 모니터링 (사진 본체 전송 X)",
        ].map((t, i, a) => ({ text: t, options: { bullet: true, breakLine: i < a.length - 1 } })),
        {
            x: 0.7, y: 4.70, w: 5.6, h: 2.4,
            fontFace: FONT_B, fontSize: 11, color: C.text,
            margin: 0, paraSpaceAfter: 4, valign: "top",
        }
    );

    // 우측 — 데이터 평면
    s.addShape(pres.shapes.RECTANGLE, {
        x: 6.8, y: 1.7, w: 6.0, h: 5.5,
        fill: { color: C.white }, line: { color: C.devStorage, width: 2 },
    });
    s.addText([
        { text: "⑥ 데이터 평면", options: { bold: true, fontSize: 18, color: C.devStorage, breakLine: true } },
        { text: "클라/Vision PC ↔ 데이터 보관 PC (직접)", options: { fontSize: 12, color: C.textMuted } },
    ], {
        x: 7.0, y: 1.85, w: 5.6, h: 0.7,
        fontFace: FONT_H, margin: 0, valign: "top",
    });
    s.addText([
        { text: "프로토콜 ", options: { bold: true, color: C.textMuted, fontSize: 11, breakLine: false } },
        { text: "보관 PC HTTP · 8004 (Drogon C++ 미니 서버)", options: { color: C.text, fontSize: 11, breakLine: true } },
        { text: "역할 ", options: { bold: true, color: C.textMuted, fontSize: 11, breakLine: false } },
        { text: "사진 본체 PUT/GET — 메인서버 우회 + HMAC 토큰 검증 (op·sub·jti·mime·exp)", options: { color: C.text, fontSize: 11, breakLine: true } },
        { text: "포맷 ", options: { bold: true, color: C.textMuted, fontSize: 11, breakLine: false } },
        { text: "image/jpeg · image/png 바이너리 (atomic write — .tmp → rename)", options: { color: C.text, fontSize: 11, breakLine: true } },
    ], {
        x: 7.0, y: 2.8, w: 5.6, h: 1.5,
        fontFace: FONT_B, margin: 0, valign: "top", paraSpaceAfter: 6,
    });
    s.addText("보관 PC 엔드포인트 (구현 완료)", {
        x: 7.0, y: 4.35, w: 5.6, h: 0.3,
        fontFace: FONT_H, fontSize: 12, bold: true, color: C.primary, margin: 0,
    });
    s.addText(
        [
            "PUT /storage/photos/{anon}/{photo_id}.{ext} — 클라 직접 업로드",
            "GET /storage/photos/{anon}/{photo_id}.{ext} — Vision PC 다운로드",
            "GET /health — 디스크 여유 + storage_root 존재 확인",
            "11가지 보안 검증 (서명·iss·aud·exp·op·sub·jti·mime·max·CT·경로)",
            "파일 경로: <root>/<anonymous_id>/<photo_id>.<ext>",
        ].map((t, i, a) => ({ text: t, options: { bullet: true, breakLine: i < a.length - 1 } })),
        {
            x: 7.0, y: 4.70, w: 5.6, h: 2.4,
            fontFace: FONT_B, fontSize: 11, color: C.text,
            margin: 0, paraSpaceAfter: 4, valign: "top",
        }
    );
}

// =====================================================
// 슬라이드 9 — 통신 ⑦ 메인서버 ↔ 식약처 OpenAPI
// =====================================================
addCommSlide({
    eyebrow: "COMMUNICATION ⑦",
    title: "메인 서버 ↔ 식약처 OpenAPI (외부)",
    deviceA: "메인 서버 PC\n10.10.10.97",
    deviceB: "식약처 OpenAPI\napis.data.go.kr",
    colorA: C.devServer, colorB: C.devExternal,
    arrowColor: C.arrowHTTPS,
    protocol: "HTTPS (TLS 1.2+) — 공공데이터포털 REST",
    port: "TCP 443 (아웃바운드만)",
    format: "JSON 또는 XML 응답",
    direction: "메인 서버 → 식약처 단방향 호출 (캐시 미스 시에만)",
    dataItems: [
        "낱알식별 정보 — 모양·색·제조사·각인 (운영 중 호출 X — CSV/스마트싱크 일괄 적재)",
        "DUR 품목 정보 — 병용금기·연령금기 등 (일괄 적재)",
        "DUR 성분 정보 — 성분 단위 안전정보 (일괄 적재)",
        "e약은요 — 효능·복용법·주의사항 (사용자 요청 시점에 lazy 캐싱, 만료 정책 없음)",
    ],
    scenarios: [
        "사용자가 처음 보는 약을 풀에 등록 → 메인이 drug_overview 에 캐시 미스 → 식약처 API 호출 → DB 적재 → 응답",
        "동일 약을 다음 사용자가 요청 → 캐시 hit → 외부 호출 없음",
        "정기 갱신 — 낱알식별·DUR 데이터는 식약처 CSV/스마트싱크로 월 1회 또는 수동 갱신",
    ],
    note: "운영 중 외부 API 호출은 거의 발생하지 않음 — 외부 의존성 최소화 + 가용성 ↑",
});

// =====================================================
// 슬라이드 10 — 요약 + 핵심 설계 원칙
// =====================================================
{
    const s = pres.addSlide();
    addSlideHeader(s, "DESIGN PRINCIPLES", "핵심 설계 원칙 — 한눈에");

    const cards = [
        {
            title: "5대 + 폰 분리 구성",
            body:  "메인 / 데이터보관 / LLM / Vision / 클라 + 폰 — 카테고리별 분리, 학습+추론 동거 (네트워크 분리 가능 설계)",
            color: C.primary,
        },
        {
            title: "추론 카테고리별 분리",
            body:  "LLM PC (8002) · Vision PC (8003) 분리 — 워크로드 특성·메모리 프로파일 상이",
            color: C.devLLM,
        },
        {
            title: "가명화 단일-컷 매핑",
            body:  "user_id → anonymous_id 단일 끊기. PIPA·GDPR 가명처리 직접 준수",
            color: C.devServer,
        },
        {
            title: "사진·음성 분리 저장",
            body:  "사진 = 데이터 보관 PC 전용 / 음성 = 폰 외부 비송신 + STT 텍스트만 흐름 (DB 비저장)",
            color: C.devStorage,
        },
        {
            title: "컨트롤 ↔ 데이터 평면 분리",
            body:  "메인 서버 = 메타·토큰 발급 / 클라·Vision PC = 데이터 보관 PC 직접 PUT·GET (AWS S3+RDS 패턴)",
            color: C.devVision,
        },
        {
            title: "의료 안내 LLM·RAG 금지",
            body:  "DUR 위험 안내는 정해진 템플릿 + 식약처 데이터 인용. RAG 허용 영역 = Onboarding + 비의료 일반 안내",
            color: C.accent,
        },
    ];

    const startX = 0.5, startY = 1.6;
    const cardW = 4.0, cardH = 1.55, gapX = 0.2, gapY = 0.3;
    cards.forEach((c, idx) => {
        const col = idx % 3, row = Math.floor(idx / 3);
        const x = startX + col * (cardW + gapX);
        const y = startY + row * (cardH + gapY);

        s.addShape(pres.shapes.RECTANGLE, {
            x, y, w: cardW, h: cardH,
            fill: { color: C.white }, line: { color: C.line, width: 1 },
            shadow: { type: "outer", color: "000000", blur: 6, offset: 2, angle: 90, opacity: 0.08 },
        });
        s.addShape(pres.shapes.RECTANGLE, {
            x, y, w: 0.1, h: cardH,
            fill: { color: c.color }, line: { color: c.color, width: 0 },
        });
        s.addText(c.title, {
            x: x + 0.25, y: y + 0.15, w: cardW - 0.35, h: 0.4,
            fontFace: FONT_H, fontSize: 14, bold: true,
            color: C.text, margin: 0,
        });
        s.addText(c.body, {
            x: x + 0.25, y: y + 0.58, w: cardW - 0.35, h: cardH - 0.65,
            fontFace: FONT_B, fontSize: 11,
            color: C.text, margin: 0, valign: "top",
        });
    });

    // 하단 참조
    s.addShape(pres.shapes.RECTANGLE, {
        x: 0.5, y: 6.85, w: 12.3, h: 0.55,
        fill: { color: "1A3454" }, line: { color: "1A3454", width: 0 },
    });
    s.addText([
        { text: "정본 문서 ", options: { bold: true, color: C.accent, fontSize: 11 } },
        { text: "  ·  시스템 흐름 정리본 v3.0", options: { color: C.white, fontSize: 11 } },
        { text: "  ·  시스템_연결구조 v2.2", options: { color: C.white, fontSize: 11 } },
        { text: "  ·  Api/ApiOverview v0.3", options: { color: C.white, fontSize: 11 } },
        { text: "  ·  DB_ERD v4", options: { color: C.white, fontSize: 11 } },
        { text: "  ·  TestMode.md", options: { color: C.white, fontSize: 11 } },
    ], {
        x: 0.7, y: 6.9, w: 12.0, h: 0.45,
        fontFace: FONT_B, margin: 0, valign: "middle",
    });
}

// =====================================================
// 출력
// =====================================================
pres.writeFile({ fileName: "MediBridge_Architecture_MVP.pptx" })
    .then(() => console.log("OK"))
    .catch(e => { console.error("ERROR:", e); process.exit(1); });
