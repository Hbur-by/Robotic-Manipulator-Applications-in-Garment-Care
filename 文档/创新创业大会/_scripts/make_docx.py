# -*- coding: utf-8 -*-
"""
生成《创意创新作品简介》—— 电气与电子工程学院报送件。
版式严格对齐 附件1.创意创新作品展示报送规范.docx / 电气与电子工程学院-.docx：
  标题 黑体 18pt
  一级小标题 黑体 16pt
  正文 宋体 16pt
  图注 宋体 14pt 居中
A4 页面，左右 1.25in、上下 1in 页边距。
四张图按附件1模板的 2×2 图位嵌入，图片文件与图注同名。
"""

import os
from docx import Document
from docx.shared import Pt, Cm, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT, WD_ALIGN_VERTICAL, WD_ROW_HEIGHT_RULE
from docx.oxml.ns import qn
from docx.oxml import OxmlElement

HEI = "黑体"
SONG = "宋体"

HERE = os.path.dirname(os.path.abspath(__file__))
OUT_DIR = os.path.abspath(os.path.join(
    HERE, "..", "报送", "电气与电子工程学院－一体化智能衣物管家机器人"))
OUT_DOCX = os.path.join(OUT_DIR, "作品简介.docx")
IMG_DIR = os.path.join(OUT_DIR, "图片")
IMG_WIDTH = Cm(6.4)
COL_WIDTH = Cm(6.65)

# 同时把填好的内容写进学院下发的模板文件本身
# （原件已备份为 电气与电子工程学院-原始模板备份.docx）
TEMPLATE_DOCX = os.path.abspath(os.path.join(HERE, "..", "电气与电子工程学院-.docx"))
OUT_ALL = [OUT_DOCX, TEMPLATE_DOCX]


def set_run(run, font=SONG, size=16, bold=False, color=None):
    run.font.name = font
    run.font.size = Pt(size)
    run.font.bold = bold
    if color is not None:
        run.font.color.rgb = color
    rPr = run._element.get_or_add_rPr()
    rFonts = rPr.find(qn("w:rFonts"))
    if rFonts is None:
        rFonts = OxmlElement("w:rFonts")
        rPr.append(rFonts)
    rFonts.set(qn("w:ascii"), font)
    rFonts.set(qn("w:hAnsi"), font)
    rFonts.set(qn("w:eastAsia"), font)
    rFonts.set(qn("w:cs"), font)
    return run


def add_para(doc, text="", font=SONG, size=16, bold=False, align=None,
             first_indent=None, space_before=0, space_after=0,
             line_spacing=1.5, color=None):
    p = doc.add_paragraph()
    pf = p.paragraph_format
    pf.space_before = Pt(space_before)
    pf.space_after = Pt(space_after)
    pf.line_spacing = line_spacing
    if first_indent is not None:
        pf.first_line_indent = Cm(first_indent)
    if align is not None:
        p.alignment = align
    if text:
        set_run(p.add_run(text), font=font, size=size, bold=bold, color=color)
    return p


def set_cell_borders(cell, sz=6, color="808080"):
    tcPr = cell._tc.get_or_add_tcPr()
    borders = OxmlElement("w:tcBorders")
    for edge in ("top", "left", "bottom", "right"):
        el = OxmlElement("w:" + edge)
        el.set(qn("w:val"), "dashed" if edge in ("top", "bottom", "left", "right") else "single")
        el.set(qn("w:sz"), str(sz))
        el.set(qn("w:space"), "0")
        el.set(qn("w:color"), color)
        borders.append(el)
    tcPr.append(borders)


def set_cell_shading(cell, fill="F2F2F2"):
    tcPr = cell._tc.get_or_add_tcPr()
    shd = OxmlElement("w:shd")
    shd.set(qn("w:val"), "clear")
    shd.set(qn("w:color"), "auto")
    shd.set(qn("w:fill"), fill)
    tcPr.append(shd)


def cell_text(cell, text, font=SONG, size=10.5, bold=False, align=WD_ALIGN_PARAGRAPH.CENTER,
              color=None):
    cell.text = ""
    p = cell.paragraphs[0]
    p.alignment = align
    pf = p.paragraph_format
    pf.space_before = Pt(2)
    pf.space_after = Pt(2)
    pf.line_spacing = 1.15
    set_run(p.add_run(text), font=font, size=size, bold=bold, color=color)


def caption(cell, text):
    p = cell.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    pf = p.paragraph_format
    pf.space_before = Pt(0)
    pf.space_after = Pt(0)
    pf.line_spacing = 1.0
    set_run(p.add_run(text), font=SONG, size=14)


def set_table_borders_none(table):
    tblPr = table._tbl.tblPr
    old = tblPr.find(qn("w:tblBorders"))
    if old is not None:
        tblPr.remove(old)
    borders = OxmlElement("w:tblBorders")
    for edge in ("top", "left", "bottom", "right", "insideH", "insideV"):
        el = OxmlElement("w:" + edge)
        el.set(qn("w:val"), "nil")
        borders.append(el)
    layout = tblPr.find(qn("w:tblLayout"))
    if layout is not None:
        layout.addprevious(borders)
    else:
        tblPr.append(borders)


def set_cell_margins(cell, twips=14):
    tcPr = cell._tc.get_or_add_tcPr()
    mar = OxmlElement("w:tcMar")
    for side in ("top", "left", "bottom", "right"):
        el = OxmlElement("w:" + side)
        el.set(qn("w:w"), str(twips))
        el.set(qn("w:type"), "dxa")
        mar.append(el)
    tcPr.append(mar)


def add_image_cell(cell, img_path, title):
    if not os.path.exists(img_path):
        raise FileNotFoundError("缺少图片：" + img_path)
    set_cell_margins(cell)
    cell.vertical_alignment = WD_ALIGN_VERTICAL.CENTER
    p = cell.paragraphs[0]
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    pf = p.paragraph_format
    pf.space_before = Pt(0)
    pf.space_after = Pt(0)
    pf.line_spacing = 1.0
    run = p.add_run()
    inline = run.add_picture(img_path, width=IMG_WIDTH)
    docPr = inline._inline.docPr
    docPr.set("title", title)
    docPr.set("descr", title)
    caption(cell, title)


def main():
    doc = Document()

    # ---- 页面：A4 + 边距与模板一致 ----
    sec = doc.sections[0]
    sec.page_width = Cm(21.0)
    sec.page_height = Cm(29.7)
    sec.left_margin = Cm(3.175)
    sec.right_margin = Cm(3.175)
    sec.top_margin = Cm(2.54)
    sec.bottom_margin = Cm(2.54)

    # 默认样式
    normal = doc.styles["Normal"]
    normal.font.name = SONG
    normal.font.size = Pt(16)
    normal.element.rPr.rFonts.set(qn("w:eastAsia"), SONG)

    # ================= 标题 =================
    add_para(doc, "电气与电子工程学院创意创新作品简介",
             font=HEI, size=18, align=WD_ALIGN_PARAGRAPH.CENTER,
             space_after=12, line_spacing=1.0)

    # ================= 一、作品名称 =================
    add_para(doc, "一、作品名称", font=HEI, size=16, space_before=6, space_after=2)
    add_para(doc, "一体化智能衣物管家机器人", font=SONG, size=16, bold=True,
             space_after=6)

    # ================= 二、团队成员 =================
    add_para(doc, "二、作品作者/团队成员", font=HEI, size=16, space_before=6, space_after=2)
    add_para(doc, "团队成员：韩帛轩、＿＿＿＿＿＿", font=SONG, size=16, space_after=0)
    add_para(doc, "指导教师：杨宇兵", font=SONG, size=16, space_after=6)

    # ================= 三、作品简介 =================
    add_para(doc, "三、作品简介", font=HEI, size=16, space_before=6, space_after=2)

    intro = (
        "随着居民生活品质提升，衣物护理需求正由整体除湿向部位级精细化作业演进。市面主流设备多采用固定式末端，"
        "仅能对整柜衣物执行统一的除湿、香薰与消毒，既无法对领口、腋下、袖口等易出汗部位进行靶向处理，也难以在熨烫、"
        "夹持、局部送风等工序间快速切换，导致护理效果不均、能耗偏高。本作品以可伸展的多自由度机械臂替代固定末端，"
        "构建“感知—决策—执行—交互”分层架构：感知层由单目相机实时采集工作区图像；决策层完成目标检测、像素坐标到"
        "关节角度的映射与轨迹生成；执行层由ESP32-S3单芯片驱动PCA9685输出六路PWM，协调六轴金属机械臂与滑轨平台的"
        "复合运动，并按需通断除湿风机、香薰雾化与UV消毒负载；交互层通过内置网页服务器向手机与电脑提供实时画面、"
        "状态监控与一键作业指令。系统已完成运动学建模（D-H参数、正逆解推导与工作空间分析）与视觉引导闭环验证："
        "在前期验证平台上，目标识别—机械臂对位—局部作业的完整闭环在20次测试中成功18次，平均端到端耗时8.8秒，"
        "验证了以低成本单目视觉替代高精度伺服反馈实现部位级靶向护理的可行性。机械臂末端采用标准化快换接口结构方案，"
        "通过定位销与滚珠自锁机构实现2~3秒换头，为除湿、熨烫、香薰、UV消毒、柔性夹持等多工序柔性作业预留机械与"
        "电气接口，形成“一臂多用”的紧凑集成方案。"
    )
    add_para(doc, intro, font=SONG, size=16, first_indent=0.85, line_spacing=1.5,
             space_after=0)

    # ================= 四张图（2×2，按附件1模板） =================
    fig_titles = [
        "作品示意图",
        "系统架构图",
        "视觉引导定位流程图",
        "机械臂工作空间与运动学验证图",
    ]

    table = doc.add_table(rows=2, cols=2)
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    table.autofit = False
    set_table_borders_none(table)
    # fixed layout + explicit table width (2 x 6.65 cm = 13.3 cm)
    tblPr = table._tbl.tblPr
    tblW = tblPr.find(qn("w:tblW"))
    if tblW is None:
        tblW = OxmlElement("w:tblW")
        tblPr.insert(0, tblW)
    tblW.set(qn("w:w"), "7540")
    tblW.set(qn("w:type"), "dxa")
    for col in table.columns:
        col.width = COL_WIDTH
    for row in table.rows:
        row.height = Cm(4.9)
        row.height_rule = WD_ROW_HEIGHT_RULE.AT_LEAST
        trPr = row._tr.get_or_add_trPr()
        cantSplit = OxmlElement("w:cantSplit")
        trPr.append(cantSplit)
        for cell in row.cells:
            cell.width = COL_WIDTH

    for idx, title in enumerate(fig_titles):
        cell = table.cell(idx // 2, idx % 2)
        add_image_cell(cell, os.path.join(IMG_DIR, title + ".png"), title)

    # ================= 四、作品创新点 =================
    add_para(doc, "四、作品创新点", font=HEI, size=16, space_before=2, space_after=2)
    doc.paragraphs[-1].paragraph_format.keep_with_next = False
    doc.paragraphs[-1].paragraph_format.widow_control = False

    points = [
        ("1. 部位级靶向护理的控制框架。",
         "以可伸展机械臂替代固定末端，将“整柜统一作业”细化为“对领口、腋下等关键部位的定点作业”，"
         "末端可在工作空间内任意位姿停驻并局部送风，使干燥与消毒作用集中于易出汗区域，在同等能耗下提升护理均匀性。"),
        ("2. 低成本单目视觉引导定位方法。",
         "采用单目俯视相机配合高对比靶标与HSV阈值分割提取部位像素坐标，经网格查表粗定位与增量闭环微调逐步收敛，"
         "不依赖高精度伺服反馈与昂贵视觉传感器，整套视觉方案成本控制在百元级，便于规模化推广。"),
        ("3. 单芯片一体化控制架构。",
         "ESP32-S3单芯片同时承担六路舵机PWM生成、图像采集与目标识别、WiFi网页交互与状态机调度，"
         "省去上位机与工控机，显著降低整机BOM成本与布线复杂度，为家用场景的产品化提供可行路径。"),
        ("4. 标准化快换接口结构方案。",
         "设计主盘与工具盘分离结构，由两根圆柱定位销与防呆缺口保证重复定位精度，弹簧滚珠自锁实现插入自动锁紧、"
         "按压快速解锁，换头时间2~3秒，并集成电气触点与气路/水路接口，支撑多工序柔性自动化作业。"),
    ]
    for head, body in points:
        p = doc.add_paragraph()
        pf = p.paragraph_format
        pf.first_line_indent = Cm(0.85)
        pf.line_spacing = 1.5
        pf.space_after = Pt(0)
        pf.widow_control = False
        pf.keep_together = False
        set_run(p.add_run(head), font=SONG, size=16, bold=True)
        set_run(p.add_run(body), font=SONG, size=16)

    # ================= 五、获得荣誉 =================
    add_para(doc, "五、获得荣誉", font=HEI, size=16, space_before=4, space_after=2)
    add_para(doc, "1. 高校智能机器人创意大赛  国家级三等奖", font=SONG, size=16,
             space_after=2)

    for p in OUT_ALL:
        os.makedirs(os.path.dirname(p), exist_ok=True)
        doc.save(p)
        print("SAVED:", p)


if __name__ == "__main__":
    main()