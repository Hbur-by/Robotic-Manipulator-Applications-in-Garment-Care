# -*- coding: utf-8 -*-
"""
生成《硬件体检表》—— 逐项实测、边测边填。
H0-H8 共 9 项，每项含：测什么 / 怎么测 / 合格判据 / 实测记录 / 通过? / 不过怎么办。
"""

import os
from openpyxl import Workbook
from openpyxl.styles import Font, Alignment, PatternFill, Border, Side
from openpyxl.utils import get_column_letter
from openpyxl.worksheet.datavalidation import DataValidation

OUT = r"文档\创新创业大会\报送\电气与电子工程学院－一体化智能衣物管家机器人\硬件体检表.xlsx"

THIN = Side(style="thin", color="A6A6A6")
BORDER = Border(left=THIN, right=THIN, top=THIN, bottom=THIN)
HDR_FILL = PatternFill("solid", fgColor="1F4E79")
HDR_FONT = Font(name="微软雅黑", size=11, bold=True, color="FFFFFF")
TITLE_FONT = Font(name="微软雅黑", size=16, bold=True, color="1F4E79")
SEC_FONT = Font(name="微软雅黑", size=11, bold=True, color="1F4E79")
CELL_FONT = Font(name="微软雅黑", size=10)
WRAP = Alignment(wrap_text=True, vertical="top", horizontal="left")
CENTER = Alignment(wrap_text=True, vertical="center", horizontal="center")

ROWS = [
    ("H0", "结构与装配", "手推末端各方向测旷量；逐颗螺丝紧固；检查底座配重；观察线缆走向是否被关节拉扯",
     "末端旷量 < 2mm；推不倒；全行程中无拉扯、无异响",
     "加配重块 / 扎带固定 / 换长线（半天）"),
    ("H1", "舵机电源带载电压", "万用表测 6V 轨电压：①空载 ②6 轴同时缓慢运动时峰值最低值",
     "带载 ≥ 5.5V；ESP32 不复位、不重启、WiFi 不掉",
     "换 6V/≥10A 电源；或同动≤3 轴分时驱动；并 4700µF 电解电容"),
    ("H2", "供电拓扑", "查 PCA9685 的 V+ 跳线是否断开；查舵机电源与 ESP32 是否共地；查 ESP32 是否由 USB 独立供电",
     "V+ 由舵机电源独立供电；共地；ESP32 走 USB，不走舵机电源",
     "断 V+ 跳线改独立供电（BOM 已标“电源模块损坏”，多半是这里）"),
    ("H3", "PCA9685 驱动板", "I2C 扫描确认地址 0x40；逐通道 CH0–CH6 输出 50Hz 脉宽（示波器或接舵机试）",
     "7 个通道（6 轴 + 滑轨）全部有输出，脉宽 0.5–2.9ms 可调",
     "换模块（¥15，建议一次买 2 个备用）"),
    ("H4", "六个舵机本体", "CH0–CH5 逐轴 ±10° 点动 10 次，手摸温度、听异响、看迟滞",
     "无迟滞、无异响、连续点动后不烫手（<50℃）",
     "更换对应舵机（备 2 个 MG996R）"),
    ("H5", "行程与限位实测", "每关节从安全区（60°–180°）缓慢向两端加角度，记录堵转点、机械干涉点、线缆拉直点",
     "得到 6 组实测 JOINT_MIN / JOINT_MAX，写入 config.h",
     "先只在 60°–180° 安全区内作业，限位后补测"),
    ("H6", "滑轨通道 CH6", "给 CH6 打 1.0ms / 1.5ms / 2.0ms 三种脉宽，看电机正转 / 停止 / 反转，并测带载能否拉动",
     "三态可分、方向正确、带载有力、限位开关（若有）有效",
     "砍掉滑轨支线，主线（L0–L6）不受影响"),
    ("H7", "主控与固件", "PlatformIO 烧录；确认 PSRAM 是否启用；WiFi AP 是否起；浏览器能否打开网页；MJPEG 有无画面",
     "烧录成功；网页可开；实时画面正常；PSRAM 可用",
     "PSRAM 按 README 5.2 的 qio_opi 配置；摄像头不行就换 USB 相机走笔记本"),
    ("H8", "安全急停", "把船型开关串进舵机电源；运动中按下",
     "按下后 0.5s 内全部舵机失力，机械臂安全停住",
     "今天必须加上（现场有观众）"),
]

EXTRA = [
    ("X1", "型号与数量核对", "清点：ESP32-S3 N16R8 ×?、PCA9685 ×?、MG996R ×?、6V 电源 ×?、OV2640 ×?、USB 相机 ×?",
     "清单与实物一致，缺口已下单", "缺件立即下单（选芜湖本地或次日达）"),
    ("X2", "备件与耗材", "备舵机 2 个、PCA9685 1 个、扎带、热缩管、杜邦线、万用表电池、电工胶带",
     "全部到位", "缺项在开工前补齐"),
]


def build():
    wb = Workbook()
    ws = wb.active
    ws.title = "硬件体检表"

    headers = ["编号", "查什么", "怎么测", "合格判据", "实测记录（填数值/现象）", "通过?", "不过怎么办"]
    widths = [8, 18, 42, 34, 30, 8, 34]

    ws.merge_cells("A1:G1")
    c = ws["A1"]
    c.value = "一体化智能衣物管家机器人 · 硬件体检表"
    c.font = TITLE_FONT
    c.alignment = CENTER
    ws.row_dimensions[1].height = 30

    ws.merge_cells("A2:G2")
    c = ws["A2"]
    c.value = "填表说明：按 H0→H8 顺序测，一边测一边在“实测记录”里填数值或现象；“通过?”填 是/否。判读规则见表格下方。"
    c.font = Font(name="微软雅黑", size=9, italic=True, color="595959")
    c.alignment = WRAP
    ws.row_dimensions[2].height = 22

    ws.append([])
    r0 = 4
    for i, h in enumerate(headers):
        cell = ws.cell(row=r0, column=i + 1, value=h)
        cell.font = HDR_FONT
        cell.fill = HDR_FILL
        cell.alignment = CENTER
        cell.border = BORDER
    for i, w in enumerate(widths):
        ws.column_dimensions[get_column_letter(i + 1)].width = w
    ws.row_dimensions[r0].height = 26

    r = r0 + 1
    for code, what, how, crit, fallback in ROWS:
        vals = [code, what, how, crit, "", "", fallback]
        for i, v in enumerate(vals):
            cell = ws.cell(row=r, column=i + 1, value=v)
            cell.font = CELL_FONT
            cell.border = BORDER
            cell.alignment = CENTER if i in (0, 5) else WRAP
        ws.row_dimensions[r].height = 58
        r += 1

    # 附加项分隔
    ws.merge_cells(start_row=r, start_column=1, end_row=r, end_column=7)
    cell = ws.cell(row=r, column=1, value="附加核对（不影响阶梯判定，但开工前必须做完）")
    cell.font = SEC_FONT
    cell.fill = PatternFill("solid", fgColor="DEEAF6")
    cell.alignment = Alignment(vertical="center", horizontal="left")
    cell.border = BORDER
    ws.row_dimensions[r].height = 22
    r += 1

    for code, what, how, crit, fallback in EXTRA:
        vals = [code, what, how, crit, "", "", fallback]
        for i, v in enumerate(vals):
            cell = ws.cell(row=r, column=i + 1, value=v)
            cell.font = CELL_FONT
            cell.border = BORDER
            cell.alignment = CENTER if i in (0, 5) else WRAP
        ws.row_dimensions[r].height = 46
        r += 1

    # 下拉：是/否
    dv = DataValidation(type="list", formula1='"是,否"', allow_blank=True)
    ws.add_data_validation(dv)
    for row in range(r0 + 1, r):
        dv.add(ws.cell(row=row, column=6))

    # 结论区
    r += 1
    ws.merge_cells(start_row=r, start_column=1, end_row=r, end_column=7)
    cell = ws.cell(row=r, column=1, value="判读规则与结论（测完填这里）")
    cell.font = SEC_FONT
    cell.fill = PatternFill("solid", fgColor="DEEAF6")
    cell.alignment = Alignment(vertical="center", horizontal="left")
    cell.border = BORDER
    r += 1

    rules = [
        ("判定规则 A", "H1 / H3 / H4 任一为“否” → 硬件主线降为 L0（只能点动示教），先修电源与驱动"),
        ("判定规则 B", "H1–H5 全部为“是” → 可推进到 L2（平滑动作 + 软限位）、L3（多套动作 + 网页一键）"),
        ("判定规则 C", "H6 为“是” → 滑轨支线 S1/S2 开启；为“否” → 直接砍掉，不影响主线"),
        ("判定规则 D", "H7 为“是” → 视觉线（L5/L6）允许启动；为“否” → 先解决摄像头与网页，视觉线挂起"),
        ("最终结论", "本次可达最高阶梯：L______     主要卡点：______________________________"),
        ("安全底线", "H8 必须为“是”，否则不得上台通电动作"),
    ]
    for k, v in rules:
        ws.cell(row=r, column=1, value=k).font = Font(name="微软雅黑", size=10, bold=True)
        ws.cell(row=r, column=1).alignment = CENTER
        ws.cell(row=r, column=1).border = BORDER
        ws.merge_cells(start_row=r, start_column=2, end_row=r, end_column=7)
        cell = ws.cell(row=r, column=2, value=v)
        cell.font = CELL_FONT
        cell.alignment = Alignment(vertical="center", horizontal="left", wrap_text=True)
        cell.border = BORDER
        ws.row_dimensions[r].height = 24
        r += 1

    ws.freeze_panes = "A5"

    # 打印设置
    ws.print_area = f"A1:G{r-1}"
    ws.page_setup.orientation = "landscape"
    ws.page_setup.paperSize = ws.PAPERSIZE_A4
    ws.page_setup.fitToWidth = 1
    ws.sheet_properties.pageSetUpPr.fitToPage = True

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    wb.save(OUT)
    print("SAVED:", OUT)


if __name__ == "__main__":
    build()