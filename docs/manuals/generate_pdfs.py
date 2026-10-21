#!/usr/bin/env python3
"""Genere les deux manuels PDF de PS TIME MANAGER.

Les captures proviennent du serveur de demonstration local. Elles n'utilisent
aucune donnee de production et montrent uniquement des noms, IP, comptes,
recettes et reseaux fictifs.
"""

from __future__ import annotations

from pathlib import Path
from typing import Iterable, Sequence

from PIL import Image as PILImage
from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER
from reportlab.lib.pagesizes import A4, landscape
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import cm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (
    BaseDocTemplate,
    Flowable,
    Frame,
    Image,
    NextPageTemplate,
    PageBreak,
    PageTemplate,
    Paragraph,
    Spacer,
    Table,
    TableStyle,
)


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "output" / "pdf"
SHOTS = ROOT / "output" / "screenshots"
ASSETS = ROOT / "central-esp32" / "ui-assets"

FONT_ROOT = Path(
    "/Users/abdelaziz/.cache/codex-runtimes/codex-primary-runtime/"
    "dependencies/native/libreoffice-headless/libreoffice/LibreOfficeDev.app/"
    "Contents/Resources/fonts/truetype"
)

PAGE = landscape(A4)
PAGE_W, PAGE_H = PAGE
MARGIN_X = 1.35 * cm
MARGIN_TOP = 1.42 * cm
MARGIN_BOTTOM = 1.18 * cm

NAVY = colors.HexColor("#071426")
NAVY_2 = colors.HexColor("#0B1D32")
BLUE = colors.HexColor("#1877F2")
CYAN = colors.HexColor("#19BFE8")
PURPLE = colors.HexColor("#7447ED")
GREEN = colors.HexColor("#189A69")
AMBER = colors.HexColor("#C98618")
RED = colors.HexColor("#C93B4D")
INK = colors.HexColor("#142033")
MUTED = colors.HexColor("#5E6E82")
LINE = colors.HexColor("#D9E2EC")
PAPER = colors.HexColor("#F4F7FB")
PALE_BLUE = colors.HexColor("#EAF3FF")
PALE_AMBER = colors.HexColor("#FFF5DF")
PALE_GREEN = colors.HexColor("#EAF8F2")
WHITE = colors.white


def register_fonts() -> None:
    pdfmetrics.registerFont(TTFont("DejaVu", str(FONT_ROOT / "DejaVuSans.ttf")))
    pdfmetrics.registerFont(TTFont("DejaVu-Bold", str(FONT_ROOT / "DejaVuSans-Bold.ttf")))
    pdfmetrics.registerFont(TTFont("DejaVu-Mono", str(FONT_ROOT / "DejaVuSansMono.ttf")))
    pdfmetrics.registerFontFamily(
        "DejaVu", normal="DejaVu", bold="DejaVu-Bold", italic="DejaVu", boldItalic="DejaVu-Bold"
    )


register_fonts()

BASE = getSampleStyleSheet()
STYLES = {
    "cover_kicker": ParagraphStyle(
        "cover_kicker", parent=BASE["Normal"], fontName="DejaVu-Bold", fontSize=11,
        leading=14, textColor=CYAN, spaceAfter=0.28 * cm, tracking=1.3,
    ),
    "cover_title": ParagraphStyle(
        "cover_title", parent=BASE["Title"], fontName="DejaVu-Bold", fontSize=34,
        leading=39, textColor=WHITE, spaceAfter=0.35 * cm,
    ),
    "cover_sub": ParagraphStyle(
        "cover_sub", parent=BASE["Normal"], fontName="DejaVu", fontSize=14,
        leading=20, textColor=colors.HexColor("#CBD7E7"), spaceAfter=0.6 * cm,
    ),
    "h1": ParagraphStyle(
        "h1", parent=BASE["Heading1"], fontName="DejaVu-Bold", fontSize=22,
        leading=27, textColor=INK, spaceAfter=0.22 * cm,
    ),
    "h2": ParagraphStyle(
        "h2", parent=BASE["Heading2"], fontName="DejaVu-Bold", fontSize=14,
        leading=18, textColor=NAVY, spaceBefore=0.2 * cm, spaceAfter=0.18 * cm,
    ),
    "body": ParagraphStyle(
        "body", parent=BASE["BodyText"], fontName="DejaVu", fontSize=9.2,
        leading=13.2, textColor=INK, spaceAfter=0.18 * cm,
    ),
    "small": ParagraphStyle(
        "small", parent=BASE["BodyText"], fontName="DejaVu", fontSize=7.7,
        leading=10.6, textColor=MUTED, spaceAfter=0.1 * cm,
    ),
    "caption": ParagraphStyle(
        "caption", parent=BASE["BodyText"], fontName="DejaVu", fontSize=7.4,
        leading=10, textColor=MUTED, alignment=TA_CENTER,
    ),
    "callout": ParagraphStyle(
        "callout", parent=BASE["BodyText"], fontName="DejaVu", fontSize=8.2,
        leading=11.5, textColor=INK, spaceAfter=0.18 * cm,
    ),
    "callout_title": ParagraphStyle(
        "callout_title", parent=BASE["BodyText"], fontName="DejaVu-Bold", fontSize=8.7,
        leading=11.5, textColor=NAVY, spaceAfter=0.08 * cm,
    ),
    "table": ParagraphStyle(
        "table", parent=BASE["BodyText"], fontName="DejaVu", fontSize=7.6,
        leading=10.2, textColor=INK,
    ),
    "table_head": ParagraphStyle(
        "table_head", parent=BASE["BodyText"], fontName="DejaVu-Bold", fontSize=7.7,
        leading=10.3, textColor=WHITE,
    ),
    "code": ParagraphStyle(
        "code", parent=BASE["Code"], fontName="DejaVu-Mono", fontSize=7.2,
        leading=10.2, textColor=colors.HexColor("#DFF7FF"), leftIndent=0,
    ),
    "note": ParagraphStyle(
        "note", parent=BASE["BodyText"], fontName="DejaVu", fontSize=8.1,
        leading=11.4, textColor=INK,
    ),
    "toc": ParagraphStyle(
        "toc", parent=BASE["BodyText"], fontName="DejaVu-Bold", fontSize=10,
        leading=14, textColor=NAVY, spaceAfter=0.14 * cm,
    ),
}


def p(text: str, style: str = "body") -> Paragraph:
    return Paragraph(text, STYLES[style])


def clean(text: str) -> str:
    return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def para_cell(text: object, header: bool = False) -> Paragraph:
    return p(clean(str(text)), "table_head" if header else "table")


def make_table(
    headers: Sequence[str], rows: Iterable[Sequence[object]], widths: Sequence[float],
    compact: bool = False,
) -> Table:
    data = [[para_cell(value, True) for value in headers]]
    data.extend([[para_cell(value) for value in row] for row in rows])
    table = Table(data, colWidths=widths, repeatRows=1, hAlign="LEFT")
    pad_y = 4 if compact else 5.5
    table.setStyle(
        TableStyle(
            [
                ("BACKGROUND", (0, 0), (-1, 0), NAVY),
                ("TEXTCOLOR", (0, 0), (-1, 0), WHITE),
                ("VALIGN", (0, 0), (-1, -1), "TOP"),
                ("GRID", (0, 0), (-1, -1), 0.45, LINE),
                ("LEFTPADDING", (0, 0), (-1, -1), 6),
                ("RIGHTPADDING", (0, 0), (-1, -1), 6),
                ("TOPPADDING", (0, 0), (-1, -1), pad_y),
                ("BOTTOMPADDING", (0, 0), (-1, -1), pad_y),
            ]
            + [
                ("BACKGROUND", (0, row), (-1, row), WHITE if row % 2 else PAPER)
                for row in range(1, len(data))
            ]
        )
    )
    return table


def bullet_list(items: Sequence[str], style: str = "body") -> list[Paragraph]:
    return [Paragraph(f"- {item}", STYLES[style]) for item in items]


def note_box(title: str, body: str, tone: str = "blue", width: float | None = None) -> Table:
    palette = {
        "blue": (PALE_BLUE, BLUE),
        "amber": (PALE_AMBER, AMBER),
        "green": (PALE_GREEN, GREEN),
        "red": (colors.HexColor("#FDECEF"), RED),
    }
    background, border = palette[tone]
    content = [p(clean(title), "callout_title"), p(body, "note")]
    box = Table([[content]], colWidths=[width or (PAGE_W - 2 * MARGIN_X)])
    box.setStyle(
        TableStyle(
            [
                ("BACKGROUND", (0, 0), (-1, -1), background),
                ("BOX", (0, 0), (-1, -1), 0.8, border),
                ("LEFTPADDING", (0, 0), (-1, -1), 10),
                ("RIGHTPADDING", (0, 0), (-1, -1), 10),
                ("TOPPADDING", (0, 0), (-1, -1), 8),
                ("BOTTOMPADDING", (0, 0), (-1, -1), 8),
            ]
        )
    )
    return box


def code_box(lines: Sequence[str], width: float | None = None) -> Table:
    text = "<br/>".join(clean(line).replace(" ", "&nbsp;") for line in lines)
    box = Table([[p(text, "code")]], colWidths=[width or (PAGE_W - 2 * MARGIN_X)])
    box.setStyle(
        TableStyle(
            [
                ("BACKGROUND", (0, 0), (-1, -1), NAVY),
                ("BOX", (0, 0), (-1, -1), 0.8, colors.HexColor("#284762")),
                ("LEFTPADDING", (0, 0), (-1, -1), 10),
                ("RIGHTPADDING", (0, 0), (-1, -1), 10),
                ("TOPPADDING", (0, 0), (-1, -1), 8),
                ("BOTTOMPADDING", (0, 0), (-1, -1), 8),
            ]
        )
    )
    return box


def fit_image(path: Path, max_w: float, max_h: float) -> Image:
    with PILImage.open(path) as source:
        width, height = source.size
    ratio = min(max_w / width, max_h / height)
    return Image(str(path), width=width * ratio, height=height * ratio)


def page_title(title: str, kicker: str | None = None) -> list[Flowable]:
    items: list[Flowable] = []
    if kicker:
        items.append(
            Paragraph(
                clean(kicker.upper()),
                ParagraphStyle(
                    "page_kicker", parent=STYLES["small"], fontName="DejaVu-Bold",
                    fontSize=7.4, leading=9, textColor=BLUE, tracking=1.0,
                ),
            )
        )
    items.append(p(clean(title), "h1"))
    items.append(
        Table(
            [[""]],
            colWidths=[PAGE_W - 2 * MARGIN_X],
            rowHeights=[0.07 * cm],
            style=TableStyle([("BACKGROUND", (0, 0), (-1, -1), CYAN)]),
        )
    )
    items.append(Spacer(1, 0.28 * cm))
    return items


class ArchitectureDiagram(Flowable):
    def __init__(self, width: float, height: float = 7.2 * cm):
        super().__init__()
        self.width = width
        self.height = height

    def draw_box(self, x: float, y: float, w: float, h: float, title: str, detail: str, fill) -> None:
        c = self.canv
        c.setFillColor(fill)
        c.setStrokeColor(colors.HexColor("#8EABC4"))
        c.roundRect(x, y, w, h, 8, fill=1, stroke=1)
        c.setFillColor(INK)
        c.setFont("DejaVu-Bold", 9)
        c.drawCentredString(x + w / 2, y + h - 16, title)
        c.setFillColor(MUTED)
        c.setFont("DejaVu", 6.7)
        c.drawCentredString(x + w / 2, y + 12, detail)

    def arrow(self, x1: float, y1: float, x2: float, y2: float, label: str = "") -> None:
        c = self.canv
        c.setStrokeColor(BLUE)
        c.setFillColor(BLUE)
        c.setLineWidth(1.5)
        c.line(x1, y1, x2, y2)
        if x2 >= x1:
            path = c.beginPath()
            path.moveTo(x2, y2)
            path.lineTo(x2 - 7, y2 + 3.5)
            path.lineTo(x2 - 7, y2 - 3.5)
        else:
            path = c.beginPath()
            path.moveTo(x2, y2)
            path.lineTo(x2 + 7, y2 + 3.5)
            path.lineTo(x2 + 7, y2 - 3.5)
        path.close()
        c.drawPath(path, fill=1, stroke=0)
        if label:
            c.setFillColor(MUTED)
            c.setFont("DejaVu", 6.3)
            c.drawCentredString((x1 + x2) / 2, y1 + 6, label)

    def draw(self) -> None:
        w, h = self.width, self.height
        box_w = w * 0.205
        box_h = h * 0.28
        center_w = w * 0.23
        center_h = h * 0.48
        left_x = 0
        center_x = (w - center_w) / 2
        right_x = w - box_w
        upper_y = h * 0.61
        lower_y = h * 0.09
        center_y = h * 0.29
        self.draw_box(left_x, upper_y, box_w, box_h, "Monnayeur", "COIN mesure a 3,3 V max.", PALE_AMBER)
        self.draw_box(left_x, lower_y, box_w, box_h, "Navigateur", "Admin ou utilisateur simple", PALE_BLUE)
        self.draw_box(center_x, center_y, center_w, center_h, "Centrale ESP32", "HTTP, NVS, LittleFS, mDNS", colors.HexColor("#E8F3F8"))
        self.draw_box(right_x, upper_y, box_w, box_h, "Poste ESP32 1", "Relais + TM1637 + NVS", PALE_GREEN)
        self.draw_box(right_x, lower_y, box_w, box_h, "Poste ESP32 N", "Relais + TM1637 + NVS", PALE_GREEN)
        self.arrow(left_x + box_w, upper_y + box_h / 2, center_x, center_y + center_h * 0.7, "GPIO13")
        self.arrow(left_x + box_w, lower_y + box_h / 2, center_x, center_y + center_h * 0.3, "HTTP local")
        self.arrow(center_x + center_w, center_y + center_h * 0.7, right_x, upper_y + box_h / 2, "Commandes HTTP")
        self.arrow(center_x + center_w, center_y + center_h * 0.3, right_x, lower_y + box_h / 2, "Annonces et etat")


class ManualDocTemplate(BaseDocTemplate):
    def __init__(self, filename: str, document_label: str, **kwargs):
        super().__init__(filename, pagesize=PAGE, **kwargs)
        self.document_label = document_label
        frame = Frame(
            MARGIN_X,
            MARGIN_BOTTOM,
            PAGE_W - 2 * MARGIN_X,
            PAGE_H - MARGIN_TOP - MARGIN_BOTTOM,
            leftPadding=0,
            rightPadding=0,
            topPadding=0,
            bottomPadding=0,
            id="main",
        )
        self.addPageTemplates(
            [
                PageTemplate(id="cover", frames=[frame], onPage=self.draw_cover),
                PageTemplate(id="content", frames=[frame], onPage=self.draw_content),
            ]
        )

    def draw_cover(self, canvas, doc) -> None:
        canvas.saveState()
        canvas.setFillColor(NAVY)
        canvas.rect(0, 0, PAGE_W, PAGE_H, fill=1, stroke=0)
        canvas.setFillColor(colors.HexColor("#0C2742"))
        canvas.circle(PAGE_W * 0.85, PAGE_H * 0.8, 5.4 * cm, fill=1, stroke=0)
        canvas.setFillColor(colors.HexColor("#143A62"))
        canvas.circle(PAGE_W * 0.9, PAGE_H * 0.83, 3.0 * cm, fill=1, stroke=0)
        canvas.setStrokeColor(CYAN)
        canvas.setLineWidth(2)
        canvas.line(MARGIN_X, 1.05 * cm, PAGE_W - MARGIN_X, 1.05 * cm)
        canvas.setFont("DejaVu", 7.5)
        canvas.setFillColor(colors.HexColor("#91A7BF"))
        canvas.drawString(MARGIN_X, 0.62 * cm, "PS TIME MANAGER - ESP32 - DOCUMENTATION LOCALE")
        canvas.restoreState()

    def draw_content(self, canvas, doc) -> None:
        canvas.saveState()
        canvas.setFillColor(PAPER)
        canvas.rect(0, 0, PAGE_W, PAGE_H, fill=1, stroke=0)
        canvas.setFillColor(NAVY)
        canvas.rect(0, PAGE_H - 0.78 * cm, PAGE_W, 0.78 * cm, fill=1, stroke=0)
        canvas.setFillColor(WHITE)
        canvas.setFont("DejaVu-Bold", 7.2)
        canvas.drawString(MARGIN_X, PAGE_H - 0.5 * cm, "PS TIME MANAGER")
        canvas.setFont("DejaVu", 7.0)
        canvas.drawRightString(PAGE_W - MARGIN_X, PAGE_H - 0.5 * cm, self.document_label)
        canvas.setStrokeColor(LINE)
        canvas.setLineWidth(0.6)
        canvas.line(MARGIN_X, 0.72 * cm, PAGE_W - MARGIN_X, 0.72 * cm)
        canvas.setFillColor(MUTED)
        canvas.setFont("DejaVu", 6.8)
        canvas.drawString(MARGIN_X, 0.4 * cm, "Version finale 1.1 - 25/08/2026")
        canvas.drawRightString(PAGE_W - MARGIN_X, 0.4 * cm, f"Page {canvas.getPageNumber()}")
        canvas.restoreState()


def cover_story(title: str, subtitle: str, audience: str) -> list[Flowable]:
    logo = fit_image(ASSETS / "ps-time-manager-logo-horizontal-hq.webp", 7.8 * cm, 2.8 * cm)
    metadata = Table(
        [
            [p("AUDIENCE", "table_head"), p(clean(audience), "table_head")],
            [p("PRODUIT", "table_head"), p("PS TIME MANAGER sur ESP32", "table_head")],
            [p("VERSION", "table_head"), p("Version finale 1.1 - 25/08/2026", "table_head")],
        ],
        colWidths=[2.4 * cm, 8.6 * cm],
    )
    metadata.setStyle(
        TableStyle(
            [
                ("BACKGROUND", (0, 0), (-1, -1), colors.HexColor("#0D223A")),
                ("BOX", (0, 0), (-1, -1), 0.8, colors.HexColor("#31516E")),
                ("INNERGRID", (0, 0), (-1, -1), 0.4, colors.HexColor("#31516E")),
                ("LEFTPADDING", (0, 0), (-1, -1), 8),
                ("RIGHTPADDING", (0, 0), (-1, -1), 8),
                ("TOPPADDING", (0, 0), (-1, -1), 6),
                ("BOTTOMPADDING", (0, 0), (-1, -1), 6),
            ]
        )
    )
    return [
        Spacer(1, 1.0 * cm),
        logo,
        Spacer(1, 0.8 * cm),
        p("DOCUMENTATION OFFICIELLE", "cover_kicker"),
        p(clean(title), "cover_title"),
        p(subtitle, "cover_sub"),
        Spacer(1, 0.25 * cm),
        metadata,
        Spacer(1, 0.42 * cm),
        Paragraph(
            "Toutes les captures utilisent des donnees de demonstration. Aucun SSID, compte, token, mot de passe ou historique reel n'apparait dans ce document.",
            ParagraphStyle(
                "cover_note", parent=STYLES["small"], fontName="DejaVu-Bold", fontSize=8,
                leading=11, textColor=colors.HexColor("#B8C8D8"),
            ),
        ),
        NextPageTemplate("content"),
        PageBreak(),
    ]


def screenshot_layout(path: Path, callouts: Sequence[tuple[str, str]]) -> Table:
    screenshot = fit_image(path, 19.2 * cm, 12.15 * cm)
    right: list[Flowable] = []
    for number, (title, text) in enumerate(callouts, start=1):
        badge = Table(
            [[p(str(number), "table_head"), [p(clean(title), "callout_title"), p(text, "callout")]]],
            colWidths=[0.65 * cm, 5.65 * cm],
        )
        badge.setStyle(
            TableStyle(
                [
                    ("BACKGROUND", (0, 0), (0, 0), BLUE),
                    ("VALIGN", (0, 0), (-1, -1), "TOP"),
                    ("ALIGN", (0, 0), (0, 0), "CENTER"),
                    ("LEFTPADDING", (0, 0), (-1, -1), 5),
                    ("RIGHTPADDING", (0, 0), (-1, -1), 5),
                    ("TOPPADDING", (0, 0), (-1, -1), 5),
                    ("BOTTOMPADDING", (0, 0), (-1, -1), 5),
                    ("LINEBELOW", (0, 0), (-1, 0), 0.45, LINE),
                ]
            )
        )
        right.append(badge)
        right.append(Spacer(1, 0.12 * cm))
    layout = Table([[screenshot, right]], colWidths=[19.55 * cm, 6.55 * cm], hAlign="LEFT")
    layout.setStyle(
        TableStyle(
            [
                ("VALIGN", (0, 0), (-1, -1), "TOP"),
                ("LEFTPADDING", (0, 0), (0, 0), 0),
                ("RIGHTPADDING", (0, 0), (0, 0), 8),
                ("LEFTPADDING", (1, 0), (1, 0), 7),
                ("RIGHTPADDING", (1, 0), (1, 0), 0),
                ("TOPPADDING", (0, 0), (-1, -1), 0),
                ("BOTTOMPADDING", (0, 0), (-1, -1), 0),
                ("BOX", (0, 0), (0, 0), 0.7, LINE),
                ("BACKGROUND", (1, 0), (1, 0), WHITE),
                ("BOX", (1, 0), (1, 0), 0.7, LINE),
            ]
        )
    )
    return layout


def add_screenshot_page(
    story: list[Flowable], title: str, intro: str, filename: str,
    callouts: Sequence[tuple[str, str]], kicker: str = "Interface",
) -> None:
    story.extend(page_title(title, kicker))
    story.append(p(intro, "body"))
    story.append(Spacer(1, 0.08 * cm))
    story.append(screenshot_layout(SHOTS / filename, callouts))
    story.append(Spacer(1, 0.16 * cm))
    story.append(p("Capture realisee avec des donnees fictives generees par code.", "caption"))
    story.append(PageBreak())


def build_technical_story() -> list[Flowable]:
    story: list[Flowable] = []
    story.extend(
        cover_story(
            "Guide technique d'installation et de mise en service",
            "Procedure complete pour la centrale et les postes : outils, bibliotheques, partitions, televersement USB, cablage final, configuration et recette.",
            "Installateur, integrateur, technicien de maintenance, client technique",
        )
    )

    story.extend(page_title("Parcours technique de A a Z", "Mode d'emploi du document"))
    story.append(
        make_table(
            ["Phase", "Pages concernees", "Livrable attendu"],
            [
                ["1. Preparer", "Architecture, securite, materiel et logiciels", "Atelier securise et nomenclature complete"],
                ["2. Installer", "Arduino IDE / CLI, coeur ESP32 et ArduinoJson", "Environnement reproductible avec versions figees"],
                ["3. Configurer", "Secrets, GPIO, Wi-Fi et partition centrale", "Sources adaptees au site et options de carte correctes"],
                ["4. Televerser", "Centrale puis firmware identique sur chaque poste", "Demarrage serie valide a 115200 bauds"],
                ["5. Cabler", "Monnayeur + pull-down 4,7 kOhm, relais et TM1637", "Continuite, tensions et polarites controlees hors charge"],
                ["6. Mettre en service", "Wi-Fi, premier compte, postes, utilisateurs et securite", "Systeme exploitable sur le LAN prive"],
                ["7. Recetter", "Tests fonctionnels, reprise, recette, sauvegarde et remise", "Proces-verbal de mise en service complete"],
            ],
            [4.2 * cm, 9.2 * cm, 12.7 * cm],
        )
    )
    story.append(Spacer(1, 0.3 * cm))
    story.append(
        note_box(
            "Captures et confidentialite",
            "Les interfaces illustrees sont produites par le serveur de demonstration du depot. SSID, IP, comptes, noms de postes, recette et evenements sont fictifs ; aucun equipement client n'a ete interroge.",
            "blue",
        )
    )
    story.append(Spacer(1, 0.28 * cm))
    story.append(
        note_box(
            "Ordre obligatoire",
            "Compiler et televerser avec les peripheriques et la charge de puissance debranches. Ne realiser le cablage final qu'apres validation du firmware, puis mettre sous tension par etapes.",
            "amber",
        )
    )
    story.append(PageBreak())

    story.extend(page_title("Objet, perimetre et architecture", "1 - Vue d'ensemble"))
    story.append(
        p(
            "Le systeme comprend une centrale ESP32 et un ESP32 par poste. La centrale compte les impulsions du monnayeur, conserve le solde et la recette, puis distribue du temps. Chaque poste execute les commandes, pilote un relais et affiche son propre decompte sur un TM1637."
        )
    )
    story.append(ArchitectureDiagram(PAGE_W - 2 * MARGIN_X))
    story.append(Spacer(1, 0.25 * cm))
    story.append(
        make_table(
            ["Element", "Responsabilite", "Stockage"],
            [
                ["Centrale ESP32", "Monnayeur, comptes, postes, solde, recette, interface web et API", "NVS + LittleFS interne"],
                ["ESP32 de poste", "Relais, session locale, affichage TM1637, annonce et reprise apres coupure", "NVS interne"],
                ["Navigateur local", "Administration et exploitation quotidienne", "Preference d'affichage locale uniquement"],
                ["Reseau Wi-Fi", "HTTP direct entre les ESP32 et resolution mDNS de pstm.local", "Aucune donnee metier"],
            ],
            [4.2 * cm, 14.3 * cm, 7.6 * cm],
        )
    )
    story.append(PageBreak())

    story.extend(page_title("Securite electrique et responsabilites", "2 - Avant toute intervention"))
    story.append(
        note_box(
            "Danger 230 V et charges de puissance",
            "Le GPIO ne commande jamais directement une charge. Le cablage du secteur, des protections, des fusibles et des contacts du relais doit etre realise et valide par une personne qualifiee selon les regles applicables.",
            "red",
        )
    )
    story.append(Spacer(1, 0.28 * cm))
    safety = [
        "Couper toutes les alimentations avant de cabler ou de deplacer un conducteur.",
        "Ne jamais relier le +12 V ni une sortie COIN superieure a 3,3 V au GPIO13.",
        "Mesurer la sortie COIN et confirmer qu'elle ne depasse jamais 3,3 V avant de la raccorder au GPIO13.",
        "Alimenter le TM1637 en 3,3 V pour eviter des rappels logiques a 5 V sur CLK et DIO.",
        "Verifier que le module relais accepte une commande logique 3,3 V et adapter RELAY_ACTIVE_HIGH.",
        "Maintenir l'isolation entre la logique basse tension et le circuit de puissance commute.",
        "Proteger mecaniquement les borniers, immobiliser les cables et identifier chaque conducteur.",
        "Sauvegarder la configuration avant toute mise a jour ou reinitialisation volontaire.",
    ]
    story.append(make_table(["Controle", "Exigence"], [[str(i + 1), item] for i, item in enumerate(safety)], [2.0 * cm, 24.1 * cm]))
    story.append(Spacer(1, 0.3 * cm))
    story.append(note_box("Regle absolue", "Les couleurs du dessin sont des reperes visuels. Les etiquettes serigraphiees et les mesures au multimetre font foi.", "amber"))
    story.append(PageBreak())

    story.extend(page_title("Materiel, outils et logiciels", "3 - Preparation"))
    story.append(
        make_table(
            ["Categorie", "Minimum recommande", "Remarque"],
            [
                ["Centrale", "1 ESP32 Dev Module, cable USB de donnees", "Flash 4 Mo et table partitions.csv du projet"],
                ["Par poste", "1 ESP32, 1 TM1637 4 chiffres, 1 module relais compatible 3,3 V", "Verifier courant, tension et logique du relais"],
                ["Monnayeur", "Monnayeur impulsionnel 12 V", "Sortie COIN mesuree a 3,3 V maximum"],
                ["Resistance centrale", "Pull-down 4,7 kOhm entre GPIO13 et GND", "Maintient un niveau bas stable au repos"],
                ["Alimentation", "12 V pour le monnayeur, USB pour l'ESP32", "Masses du monnayeur et de l'ESP32 communes"],
                ["Atelier", "Multimetre, tournevis, borniers, fusibles, gaine et etiquettes", "Verifier continuite et polarite hors tension"],
                ["Logiciels", "Arduino IDE 2.3.10 ou arduino-cli 1.5.1", "Moniteur serie regle a 115200 bauds"],
                ["Packages", "esp32 3.3.11 + ArduinoJson 7.4.3", "Les autres bibliotheques sont fournies par le coeur ESP32"],
            ],
            [4.0 * cm, 12.1 * cm, 10.0 * cm],
        )
    )
    story.append(Spacer(1, 0.28 * cm))
    story.append(
        note_box(
            "Versions de reference du projet",
            "La version finale a ete compilee avec Arduino IDE 2.3.10, arduino-cli 1.5.1, le coeur ESP32 3.3.11 et ArduinoJson 7.4.3. Le pilote TM1637 est integre au firmware du poste et ne demande aucune bibliotheque externe.",
            "blue",
        )
    )
    story.append(PageBreak())

    story.extend(page_title("Installer l'environnement de developpement", "4 - Arduino IDE et arduino-cli"))
    left = [
        p("Avec Arduino IDE", "h2"),
        *bullet_list(
            [
                "Installer Arduino IDE 2.3.10 (version finale verifiee).",
                "Dans Additional Boards Manager URLs, ajouter l'index officiel Espressif indique ci-dessous.",
                "Installer esp32 by Espressif Systems en version 3.3.11.",
                "Installer ArduinoJson de Benoit Blanchon en version 7.4.3.",
                "Verifier qu'un port serie apparait avec un cable USB de donnees.",
                "Ouvrir le moniteur serie a 115200 bauds apres televersement.",
            ],
            "callout",
        ),
    ]
    right = [
        p("Avec arduino-cli", "h2"),
        code_box(
            [
                "arduino-cli config add board_manager.additional_urls \\",
                "  https://espressif.github.io/arduino-esp32/package_esp32_index.json",
                "arduino-cli core update-index",
                "arduino-cli core install esp32:esp32@3.3.11",
                "arduino-cli lib install ArduinoJson@7.4.3",
                "arduino-cli board list",
            ],
            12.6 * cm,
        ),
        Spacer(1, 0.25 * cm),
        note_box(
            "Port serie",
            "Remplacer PORT_SERIE par /dev/cu.usbserial-... sous macOS, /dev/ttyUSB... sous Linux ou COM3 sous Windows.",
            "blue",
            12.6 * cm,
        ),
    ]
    columns = Table([[left, right]], colWidths=[13.0 * cm, 13.0 * cm])
    columns.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "TOP"), ("LEFTPADDING", (0, 0), (-1, -1), 8), ("RIGHTPADDING", (0, 0), (-1, -1), 8)]))
    story.append(columns)
    story.append(Spacer(1, 0.28 * cm))
    story.append(note_box("Connexion USB", "Un cable de charge uniquement alimente la carte mais ne permet pas le televersement. Utiliser un cable de donnees et, si necessaire, maintenir BOOT pendant le debut de l'envoi.", "amber"))
    story.append(Spacer(1, 0.2 * cm))
    story.append(note_box("Bibliotheques incluses", "WiFi, WebServer, DNSServer, Preferences, ESPmDNS, HTTPClient, FS, LittleFS, mbedTLS et FreeRTOS proviennent du coeur ESP32. Ne pas installer de doublons homonymes. Les avertissements ArduinoJson de compatibilite peuvent etre non bloquants.", "green"))
    story.append(PageBreak())

    story.extend(page_title("Preparer les firmwares avant compilation", "5 - Parametres critiques"))
    story.append(
        make_table(
            ["Fichier", "Constante", "Action avant deploiement"],
            [
                ["central-esp32/AppConfig.h", "POSTE_COMMAND_TOKEN", "Remplacer par un secret fort propre au site"],
                ["poste-esp32/PosteConfig.h", "COMMAND_TOKEN", "Copier exactement le meme secret que la centrale"],
                ["poste-esp32/PosteConfig.h", "RELAY_PIN", "Confirmer GPIO27 ou choisir une broche adaptee"],
                ["poste-esp32/PosteConfig.h", "RELAY_ACTIVE_HIGH", "Adapter a la logique active haute ou active basse du module"],
                ["poste-esp32/PosteConfig.h", "TM1637_CLK_PIN / DIO_PIN", "Confirmer GPIO25 et GPIO26"],
                ["central-esp32/AppConfig.h", "AP_PASSWORD", "Definir un mot de passe si le portail central ne doit pas rester ouvert"],
                ["poste-esp32/PosteConfig.h", "WIFI_SETUP_AP_PASSWORD", "Appliquer la meme politique au portail de chaque poste"],
                ["central-esp32/partitions.csv", "Table de partitions", "Conserver le fichier a cote du sketch central"],
            ],
            [6.4 * cm, 7.2 * cm, 12.5 * cm],
        )
    )
    story.append(Spacer(1, 0.28 * cm))
    story.append(code_box(["// Valeurs attendues dans les deux firmwares", "POSTE_COMMAND_TOKEN = <SECRET_COMMUN_A_REMPLACER>", "COMMAND_TOKEN       = <SECRET_COMMUN_A_REMPLACER>"]))
    story.append(Spacer(1, 0.28 * cm))
    story.append(note_box("Ne pas exposer les secrets", "Ne jamais placer le token reel, les mots de passe Wi-Fi ou une sauvegarde JSON dans un depot public, une capture d'ecran ou un ticket de support non protege.", "red"))
    story.append(PageBreak())

    story.extend(page_title("Appliquer la partition finale de la centrale", "6 - Table personnalisee obligatoire"))
    story.append(
        p(
            "Ouvrir central-esp32/central-esp32.ino depuis son dossier d'origine. Le fichier partitions.csv doit rester exactement a cote du .ino. Dans Arduino IDE, choisir Tools/Outils → Partition Scheme/Scheme de partition → Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS). Le nom du menu n'est qu'un profil de compilation : le fichier local ci-dessous remplace reellement son decoupage."
        )
    )
    story.append(
        make_table(
            ["Partition", "Offset", "Taille", "Usage"],
            [
                ["nvs", "0x9000", "20 Kio", "Wi-Fi, configuration, comptes, token et postes"],
                ["otadata", "0xE000", "8 Kio", "Selection des emplacements applicatifs"],
                ["app0", "0x10000", "1,625 Mio", "Firmware actif ; maximum physique 1 703 936 octets"],
                ["app1", "0x1B0000", "1,625 Mio", "Second emplacement applicatif"],
                ["littlefs", "0x350000", "640 Kio", "Recette persistante /revenue.bin"],
                ["coredump", "0x3F0000", "64 Kio", "Diagnostic ESP32"],
            ],
            [4.0 * cm, 4.0 * cm, 4.0 * cm, 14.1 * cm],
        )
    )
    story.append(Spacer(1, 0.24 * cm))
    story.append(note_box("Marge de la version finale", "La centrale compilee occupe 1 560 932 octets, soit environ 91,6 % de app0, et laisse environ 143 004 octets. Comparer la taille finale a 1 703 936 octets, pas au maximum plus optimiste affiche par le profil Minimal SPIFFS.", "amber"))
    story.append(Spacer(1, 0.2 * cm))
    story.append(note_box("USB uniquement", "La table reserve deux emplacements applicatifs, mais cette version n'implemente pas la mise a jour OTA. Effectuer un televersement USB complet pour ecrire la table. Erase All Flash doit rester Disabled, sauf effacement volontaire du Wi-Fi, des comptes et des postes NVS.", "red"))
    story.append(PageBreak())

    story.extend(page_title("Reglages Arduino IDE pour les deux firmwares", "7 - Comparaison avant compilation"))
    story.append(
        make_table(
            ["Reglage", "Centrale", "Poste"],
            [
                ["Board", "ESP32 Dev Module", "ESP32 Dev Module"],
                ["Flash Size", "4MB (32Mb)", "4MB (32Mb)"],
                ["Partition Scheme", "Minimal SPIFFS + partitions.csv local", "Default 4MB with spiffs"],
                ["CPU Frequency", "240 MHz (WiFi/BT)", "240 MHz (WiFi/BT)"],
                ["Flash Frequency / Mode", "80 MHz / QIO", "80 MHz / QIO"],
                ["PSRAM", "Disabled", "Disabled"],
                ["Upload Speed", "460800 ; 115200 si instable", "460800 ; 115200 si instable"],
                ["Erase All Flash", "Disabled", "Disabled"],
                ["Port", "Port USB de la centrale", "Port USB du poste concerne"],
                ["Firmware final", "1 560 932 octets ; 52 776 octets globaux", "1 134 216 octets ; 51 632 octets globaux"],
            ],
            [6.0 * cm, 10.05 * cm, 10.05 * cm],
        )
    )
    story.append(Spacer(1, 0.24 * cm))
    story.append(note_box("Controle de la cible", "Verifier le port avant chaque Upload : plusieurs ESP32 branches peuvent etre intervertis. Noter le chipId et etiqueter physiquement la carte des que le moniteur serie l'affiche.", "blue"))
    story.append(PageBreak())

    story.extend(page_title("Schema de cablage de la centrale", "8 - Monnayeur et GPIO13"))
    story.append(fit_image(SHOTS / "schema-cablage-centrale.jpg", 26.7 * cm, 14.4 * cm))
    story.append(Spacer(1, 0.12 * cm))
    story.append(p("Schema source : central-esp32/docs/schema-cablage-central-final.svg", "caption"))
    story.append(PageBreak())

    story.extend(page_title("Raccordement de la centrale, broche par broche", "9 - Controle avant mise sous tension"))
    story.append(
        make_table(
            ["Depuis", "Vers", "Fonction / controle"],
            [
                ["Alimentation +12 V", "Fil rouge du monnayeur", "Alimentation du monnayeur"],
                ["Alimentation GND", "Fil noir du monnayeur et ESP32 GND", "Masse commune"],
                ["Monnayeur blanc COIN", "ESP32 GPIO13", "Signal mesure a 3,3 V maximum"],
                ["Resistance 4,7 kOhm", "Entre GPIO13 et GND", "Pull-down : niveau bas stable au repos"],
            ],
            [8.4 * cm, 8.1 * cm, 9.6 * cm],
        )
    )
    story.append(Spacer(1, 0.25 * cm))
    checks = [
        "Hors tension : confirmer aucune continuite entre le +12 V et le GPIO13.",
        "ESP32 alimente par USB : verifier le demarrage et le moniteur serie a 115200 bauds.",
        "Hors tension : verifier la masse commune et la resistance 4,7 kOhm entre GPIO13 et GND.",
        "Monnayeur alimente : mesurer au maximum 3,3 V sur COIN avant de le relier au GPIO13.",
        "Piece test : verifier une impulsion comptee une seule fois et une recette incrementee.",
    ]
    story.append(make_table(["Etape de test", "Resultat attendu"], [[str(i + 1), value] for i, value in enumerate(checks)], [2.6 * cm, 23.5 * cm]))
    story.append(PageBreak())

    story.extend(page_title("Schema de cablage d'un poste", "10 - Relais et afficheur TM1637"))
    story.append(fit_image(SHOTS / "schema-cablage-poste.jpg", 26.7 * cm, 14.4 * cm))
    story.append(Spacer(1, 0.12 * cm))
    story.append(p("Schema source : poste-esp32/docs/schema-cablage-poste-tm1637.svg", "caption"))
    story.append(PageBreak())

    story.extend(page_title("Raccordement d'un poste et tests locaux", "11 - GPIO25, GPIO26 et GPIO27"))
    story.append(
        make_table(
            ["ESP32", "Equipement", "Fonction", "Point de vigilance"],
            [
                ["3V3", "TM1637 VCC", "Alimentation afficheur", "Ne pas alimenter CLK/DIO via des rappels a 5 V"],
                ["GND", "TM1637 GND", "Masse logique", "Masse commune avec l'ESP32"],
                ["GPIO25", "TM1637 CLK", "Horloge", "Doit differer de DIO et du relais"],
                ["GPIO26", "TM1637 DIO", "Donnees", "Doit differer de CLK et du relais"],
                ["5V/VIN ou alimentation adaptee", "Relais VCC", "Alimentation module", "Adapter au modele reel"],
                ["GND", "Relais GND", "Masse logique", "Masse commune obligatoire"],
                ["GPIO27", "Relais IN", "Commande", "Module compatible logique 3,3 V"],
                ["COM + NO", "Circuit a commander", "Contact sec", "Cablage puissance par personnel qualifie"],
            ],
            [4.1 * cm, 6.5 * cm, 5.2 * cm, 10.3 * cm],
        )
    )
    story.append(Spacer(1, 0.24 * cm))
    story.append(note_box("Test sans charge", "Avant de raccorder la machine, demarrer une session de test et verifier : TM1637 a 30:00, relais ON, decompte regulier, puis coupure automatique a zero.", "green"))
    story.append(PageBreak())

    story.extend(page_title("Compiler et televerser la centrale", "12 - Reglages Arduino et USB"))
    story.append(
        make_table(
            ["Reglage", "Valeur centrale"],
            [
                ["Sketch", "central-esp32/central-esp32.ino"],
                ["Carte", "ESP32 Dev Module ou modele compatible"],
                ["Flash Size", "4MB (32Mb)"],
                ["Partition Scheme", "Minimal SPIFFS pour laisser la place a l'application"],
                ["Fichier local", "partitions.csv conserve dans le dossier du sketch"],
                ["CPU / Flash / PSRAM", "240 MHz ; 80 MHz QIO ; PSRAM Disabled"],
                ["Upload Speed", "460800, ou 115200 en cas de liaison instable"],
                ["Effacement complet", "Desactive, sauf effacement NVS volontaire"],
                ["Moniteur serie", "115200 bauds"],
            ],
            [7.2 * cm, 18.9 * cm],
        )
    )
    story.append(Spacer(1, 0.25 * cm))
    story.append(code_box(["cd central-esp32", "arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=min_spiffs .", "arduino-cli upload --fqbn esp32:esp32:esp32:PartitionScheme=min_spiffs --port PORT_SERIE ."]))
    story.append(Spacer(1, 0.25 * cm))
    story.append(note_box("Procedure USB", "Debrancher monnayeur et peripheriques, cliquer Verifier puis Televerser. Si Connecting... persiste, maintenir BOOT jusqu'au debut de l'ecriture, relacher puis ouvrir le moniteur serie a 115200 bauds. Un televersement USB complet ecrit aussi partitions.csv.", "blue"))
    story.append(PageBreak())

    story.extend(page_title("Compiler et televerser chaque poste", "13 - Firmware poste"))
    story.append(
        make_table(
            ["Reglage", "Valeur poste"],
            [
                ["Sketch", "poste-esp32/poste-esp32.ino"],
                ["Carte", "ESP32 Dev Module ou modele compatible"],
                ["Partition Scheme", "Default 4MB with spiffs"],
                ["Bibliotheque externe", "ArduinoJson uniquement"],
                ["Afficheur", "Pilote TM1637 integre au sketch"],
                ["CPU / Flash / PSRAM", "240 MHz ; 80 MHz QIO ; PSRAM Disabled"],
                ["Moniteur serie", "115200 bauds"],
            ],
            [7.2 * cm, 18.9 * cm],
        )
    )
    story.append(Spacer(1, 0.25 * cm))
    story.append(code_box(["cd poste-esp32", "arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=default .", "arduino-cli upload --fqbn esp32:esp32:esp32:PartitionScheme=default --port PORT_SERIE ."]))
    story.append(Spacer(1, 0.25 * cm))
    story.append(note_box("Meme binaire, identites distinctes", "Le meme firmware est televerse sur tous les postes. Le chipId materiel et le nom attribue depuis la centrale rendent chaque poste unique en NVS.", "green"))
    story.append(Spacer(1, 0.2 * cm))
    story.append(note_box("Controle avant charge", "La version finale utilise 1 134 216 octets sur 1 310 720 et laisse environ 176 504 octets. Televerser charge de puissance debranchee, puis verifier 00:00 sur le TM1637 et relais OFF avant le cablage final.", "amber"))
    story.append(PageBreak())

    add_screenshot_page(
        story,
        "Mettre la centrale sur le reseau Wi-Fi",
        "Sans identifiants valides, la centrale ouvre GAMEROOM-CENTRAL-&lt;chipId&gt;. Ouvrir le portail captif ou http://192.168.4.1, puis choisir le reseau local commun aux postes.",
        "10-wifi-centrale.jpg",
        [
            ("Reseaux", "Les SSID de cette capture sont fictifs. Selectionner le reseau technique reel lors de l'installation."),
            ("DHCP", "Utiliser DHCP par defaut. Employer une IP fixe seulement avec un plan d'adressage documente."),
            ("Validation", "Apres succes, noter l'IP et tester http://pstm.local depuis un navigateur du meme LAN."),
        ],
        "14 - Mise en reseau",
    )

    add_screenshot_page(
        story,
        "Mettre un poste sur le meme reseau",
        "Un poste sans Wi-Fi valide ouvre GAMEROOM-POSTE-&lt;chipId&gt;. Le reseau choisi doit permettre les communications directes entre clients et la resolution mDNS.",
        "11-wifi-poste.jpg",
        [
            ("Reseau commun", "Choisir le meme SSID que la centrale, sans isolation des clients."),
            ("Adresse", "DHCP convient dans la majorite des salles. Reserver les adresses dans le routeur si necessaire."),
            ("Annonce", "Apres connexion, le poste resout pstm.local et s'annonce automatiquement toutes les cinq secondes."),
        ],
        "15 - Mise en reseau",
    )

    add_screenshot_page(
        story,
        "Decouvrir, nommer et verifier les postes",
        "Dans Gestion des postes, un module neuf apparait dans Postes decouverts. Cliquer Ajouter, saisir uniquement son nom visible, puis verifier IP, etat, ping et temps restant.",
        "06-gestion-des-postes.jpg",
        [
            ("Nouveau poste", "Le chipId technique est derive automatiquement et ne doit pas etre saisi manuellement."),
            ("Etats", "Actif et reprise en attente verrouillent la modification et la suppression."),
            ("Test final", "Attribuer un coin, verifier le relais et le TM1637, puis tester l'arret manuel et l'expiration."),
        ],
        "16 - Appairage",
    )

    add_screenshot_page(
        story,
        "Premiere connexion et securisation du compte",
        "Ouvrir http://pstm.local ou l'IP annoncee par la centrale. Pour une centrale neuve, utiliser provisoirement admin / 12345678, puis remplacer immediatement ce mot de passe depuis Mot de passe / Token.",
        "01-connexion.jpg",
        [
            ("Acces local", "Le navigateur doit etre sur le meme LAN. Le systeme utilise HTTP sans TLS et ne doit pas etre publie sur Internet."),
            ("Compte initial", "Le mot de passe par defaut ne doit servir qu'a la premiere connexion de mise en service."),
            ("Traçabilite", "Creer ensuite des comptes nominatifs ; ne pas partager le compte administrateur entre exploitants."),
        ],
        "17 - Configuration initiale",
    )

    add_screenshot_page(
        story,
        "Regler les coins et creer la sauvegarde initiale",
        "Dans Gestion des postes, definir le nombre de pieces requis a la creation ou via Modifier. Dans Configuration, definir la duree par attribution, la valeur monetaire et le solde en pieces. Enregistrer puis exporter une sauvegarde JSON V4 protegee.",
        "05-configuration.jpg",
        [
            ("Reglages metier", "Valider la duree, les impulsions et la valeur avec le client avant l'ouverture au public."),
            ("Solde", "Une correction manuelle n'alimente pas la recette financiere ; elle doit etre justifiee dans le suivi d'exploitation."),
            ("Sauvegarde", "Le JSON contient des donnees sensibles et pas la recette LittleFS. Le conserver chiffre, hors du poste public."),
        ],
        "18 - Configuration initiale",
    )

    add_screenshot_page(
        story,
        "Creer les utilisateurs du site",
        "Dans Utilisateurs, creer au moins un compte nominatif pour l'administrateur du site et les comptes utilisateur simple necessaires a l'exploitation. Conserver au minimum un administrateur valide.",
        "07-utilisateurs.jpg",
        [
            ("Role admin", "Accede aux reglages, recette, postes, utilisateurs, securite, journal et Wi-Fi."),
            ("Role simple", "Pilote les sessions et traite les reprises, sans acces aux donnees d'administration."),
            ("Remise", "Transmettre les identifiants par un canal protege et exiger leur changement selon la politique du client."),
        ],
        "19 - Configuration initiale",
    )

    add_screenshot_page(
        story,
        "Finaliser le mot de passe et le token API",
        "Changer le mot de passe administrateur, regenerer le token API uniquement si une integration externe est prevue, puis mettre a jour tous les consommateurs autorises. Le secret inter-ESP32 est distinct et reste dans les firmwares.",
        "08-securite.jpg",
        [
            ("Mot de passe", "Choisir une phrase longue et unique. Le changement ferme la session courante."),
            ("Token API", "Le token complet n'est visible qu'a sa regeneration. Ne jamais l'inclure dans le dossier client non protege."),
            ("Deux secrets", "Token API navigateur/integration et COMMAND_TOKEN inter-ESP32 n'ont pas le meme usage."),
        ],
        "20 - Configuration initiale",
    )

    story.extend(page_title("Reseau, API et flux entre ESP32", "21 - Communication"))
    story.append(
        make_table(
            ["Sens", "Route", "Usage"],
            [
                ["Poste vers centrale", "POST /poste/announce", "Identite, IP, etat, relais, temps et reprise"],
                ["Centrale vers poste", "GET /status", "Lecture de l'etat courant"],
                ["Centrale vers poste", "POST /configure", "Attribution ou modification de l'identite"],
                ["Centrale vers poste", "POST /identity/clear", "Retour du poste en decouverte"],
                ["Centrale vers poste", "POST /command", "START_SESSION, ADD_TIME, STOP_SESSION ou PING"],
            ],
            [6.2 * cm, 7.0 * cm, 12.9 * cm],
        )
    )
    story.append(Spacer(1, 0.25 * cm))
    story.append(
        note_box(
            "Regles reseau",
            "Tous les appareils doivent partager le meme LAN, sans isolation entre clients. mDNS doit fonctionner pour pstm.local. Le proxy HTTP n'est pas utilise : les ESP32 communiquent directement par adresse locale.",
            "blue",
        )
    )
    story.append(Spacer(1, 0.25 * cm))
    story.append(code_box(["Authorization: Bearer <TOKEN_API_MASQUE>", "Content-Type: application/json", "# Ne pas confondre token API externe et secret inter-ESP32"]))
    story.append(PageBreak())

    story.extend(page_title("Persistance, sauvegarde et maintenance", "22 - Exploitation technique"))
    story.append(
        make_table(
            ["Donnee", "Support", "Comportement"],
            [
                ["Configuration, comptes, token et postes", "NVS centrale", "Persiste apres redemarrage"],
                ["Recette", "LittleFS /revenue.bin", "Journal binaire horodate et agrege par l'interface"],
                ["Wi-Fi centrale", "NVS centrale", "Effacable par Reinitialiser le Wi-Fi"],
                ["Wi-Fi et identite poste", "NVS de chaque poste", "Persiste independamment"],
                ["Session poste", "NVS de chaque poste", "Checkpoint au debut, apres ajout et toutes les 60 s"],
                ["Sessions web", "RAM centrale", "Perdues au redemarrage"],
            ],
            [7.0 * cm, 6.0 * cm, 13.1 * cm],
        )
    )
    story.append(Spacer(1, 0.25 * cm))
    story.append(note_box("Sauvegarde JSON sensible", "Le fichier exporte peut contenir comptes, identifiants haches, token API, postes et parametres Wi-Fi/IP. Le chiffrer, limiter son acces et tester regulierement la restauration.", "amber"))
    story.append(Spacer(1, 0.25 * cm))
    story.extend(
        bullet_list(
            [
                "Sauvegarder avant une mise a jour, un changement de partitions ou une intervention NVS.",
                "Ne pas activer Erase All Flash si la conservation des donnees est requise.",
                "Verifier la synchronisation NTP avant d'interpreter les dates de recette.",
                "Surveiller l'espace LittleFS, les erreurs d'ecriture et les postes hors ligne dans le journal.",
            ]
        )
    )
    story.append(PageBreak())

    add_screenshot_page(
        story,
        "Verifier l'encaissement et les rapports",
        "Apres synchronisation de l'horloge NTP, injecter une impulsion reelle. Confirmer simultanement l'augmentation du solde, l'evenement dans le journal et l'ecriture dans Recette. Tester les periodes Jour, Semaine, Mois et les exports.",
        "04-recette.jpg",
        [
            ("Impulsions reelles", "La recette provient des impulsions physiques datees, pas des coins simules ni des corrections manuelles."),
            ("Horloge", "Les encaissements avant synchronisation restent non dates ; verifier le reseau NTP avant la recette finale."),
            ("Capacite", "Surveiller LittleFS et conserver les exports comptables selon la procedure du client."),
        ],
        "23 - Recette fonctionnelle",
    )

    story.extend(page_title("Cablage final et mise sous tension par etapes", "24 - Derniere sequence terrain"))
    story.append(
        make_table(
            ["Ordre", "Action terrain", "Critere pour continuer"],
            [
                ["1", "Couper toutes les alimentations ; verifier continuite, polarites, borniers, fusibles et absence de court-circuit", "Aucune liaison +12 V vers GPIO ; conducteurs immobilises"],
                ["2", "Brancher la centrale seule par USB, sans monnayeur ni poste de puissance", "Demarrage serie, Wi-Fi et interface web stables"],
                ["3", "Hors tension, cabler la masse commune et la resistance pull-down 4,7 kOhm", "4,7 kOhm mesure entre GPIO13 et GND"],
                ["4", "Alimenter le monnayeur sans relier COIN, puis mesurer sa sortie", "COIN ne depasse jamais 3,3 V avant raccordement au GPIO13"],
                ["5", "Alimenter un poste avec TM1637 et relais, charge debranchee", "00:00 au repos, relais OFF ; test session conforme"],
                ["6", "Couper, raccorder COM/NO a la machine par personnel qualifie, puis proteger le coffret", "Isolation logique/puissance validee"],
                ["7", "Remettre sous tension et lancer un cycle court", "Relais, affichage, decompte, arret et expiration corrects"],
                ["8", "Repeter postes un par un puis tester une coupure en cours de session", "Reprendre et Annuler fonctionnent individuellement"],
            ],
            [2.0 * cm, 15.1 * cm, 9.0 * cm],
        )
    )
    story.append(Spacer(1, 0.25 * cm))
    story.append(note_box("Ne jamais sauter le test sans charge", "Si le relais est inverse, couper l'alimentation, corriger RELAY_ACTIVE_HIGH dans PosteConfig.h, recompiler et reteleverser le poste avant de connecter la machine.", "red"))
    story.append(PageBreak())

    story.extend(page_title("Recette de mise en service", "25 - Validation et remise au client"))
    acceptance = [
        ["Alimentations", "12 V monnayeur, USB centrale et 3,3 V TM1637 mesures et stables", "OK / NOK"],
        ["Centrale", "Demarrage sans erreur, pstm.local accessible, compte admin securise", "OK / NOK"],
        ["Monnayeur", "Une piece produit le nombre d'impulsions attendu, le solde et la recette progressent", "OK / NOK"],
        ["Postes", "Tous les postes apparaissent, sont nommes, joignables et affichent 00:00 au repos", "OK / NOK"],
        ["Session", "START, ajout de temps, arret et expiration pilotent le relais et le TM1637", "OK / NOK"],
        ["Coupure", "Le relais reste OFF au redemarrage et la reprise est proposee individuellement", "OK / NOK"],
        ["Roles", "Le compte user ne voit pas les pages admin et peut exploiter les sessions", "OK / NOK"],
        ["Recette", "Jour, semaine, mois, CSV et PDF fonctionnent avec l'horloge synchronisee", "OK / NOK"],
        ["Sauvegarde", "Export stocke en lieu protege et procedure de restauration remise", "OK / NOK"],
        ["Documents", "Guide technique et manuel d'utilisation remis au client", "OK / NOK"],
    ]
    story.append(make_table(["Lot", "Critere d'acceptation", "Resultat"], acceptance, [4.2 * cm, 18.6 * cm, 3.3 * cm]))
    story.append(Spacer(1, 0.25 * cm))
    story.append(note_box("Fin de mise en service", "Consigner la date, les versions compilees, le plan IP, les numeros de poste, le lieu de la sauvegarde et le nom de la personne ayant valide le cablage de puissance.", "green"))
    return story


def build_user_story() -> list[Flowable]:
    story: list[Flowable] = []
    story.extend(
        cover_story(
            "Manuel d'utilisation",
            "Guide de deploiement et d'exploitation de A a Z : premiere ouverture, sessions, postes, recette, configuration, comptes, reseau, sauvegarde et depannage.",
            "Client final, administrateur de la salle et utilisateur simple",
        )
    )

    story.extend(page_title("Parcours complet de l'application", "Comment utiliser ce manuel"))
    story.append(
        make_table(
            ["Parcours", "Pages / interfaces", "Pour qui"],
            [
                ["Demarrer", "Connexion, roles, premiere securisation et navigation", "Administrateur et utilisateur simple"],
                ["Exploiter", "Vue des postes, affectation de coins, prolongation, arret et reprise", "Administrateur et utilisateur simple"],
                ["Controler", "Recette, graphique, tableau, CSV et PDF", "Administrateur"],
                ["Configurer", "Duree, tarifs par poste, valeur, solde, sauvegarde V4 et restauration", "Administrateur"],
                ["Administrer", "Postes, utilisateurs, mot de passe, token et journal", "Administrateur"],
                ["Raccorder au LAN", "Wi-Fi de la centrale puis Wi-Fi de chaque poste", "Administrateur accompagne de l'installateur"],
                ["Maintenir", "Ouverture/fermeture quotidienne, protection des donnees et depannage", "Responsable de site"],
            ],
            [5.0 * cm, 14.1 * cm, 7.0 * cm],
        )
    )
    story.append(Spacer(1, 0.3 * cm))
    story.append(note_box("Captures de demonstration", "Les quatre postes, utilisateurs, adresses IP, SSID, montants et evenements visibles ont ete generes par code. Ils illustrent tous les etats sans exposer de donnees client.", "blue"))
    story.append(Spacer(1, 0.25 * cm))
    story.append(note_box("Deux niveaux d'assistance", "L'utilisateur simple n'intervient jamais sur les reglages, comptes, sauvegardes ou coffrets. L'administrateur gere l'application ; le cablage et le firmware restent du ressort de l'installateur technique.", "amber"))
    story.append(PageBreak())

    story.extend(page_title("Bien demarrer et comprendre les roles", "1 - Reperes"))
    story.append(
        p(
            "Ouvrir http://pstm.local ou l'adresse IP de la centrale depuis un appareil connecte au meme reseau local. Les actions visibles dependent du role du compte. Les permissions sont controlees par la centrale, pas seulement masquees dans le navigateur."
        )
    )
    story.append(
        make_table(
            ["Fonction", "Administrateur", "Utilisateur simple"],
            [
                ["Voir les postes et le solde", "Oui", "Oui"],
                ["Affecter des coins et arreter une session", "Oui", "Oui"],
                ["Reprendre ou annuler une session interrompue", "Oui", "Oui"],
                ["Simuler une piece", "Oui", "Non"],
                ["Configurer ou supprimer des postes", "Oui", "Non"],
                ["Modifier les reglages et la sauvegarde", "Oui", "Non"],
                ["Consulter et exporter la recette", "Oui", "Non"],
                ["Gerer utilisateurs, securite et journal", "Oui", "Non"],
            ],
            [15.0 * cm, 5.55 * cm, 5.55 * cm],
        )
    )
    story.append(Spacer(1, 0.3 * cm))
    story.append(note_box("Donnees de ce manuel", "Tous les postes, comptes, IP, recettes et SSID visibles dans les captures sont fictifs. Ils ont ete generes uniquement pour illustrer l'interface.", "blue"))
    story.append(PageBreak())

    add_screenshot_page(
        story,
        "Se connecter",
        "Saisir le nom d'utilisateur et le mot de passe remis par l'administrateur. Le mot de passe reste masque. L'option Se souvenir de moi memorise uniquement le nom d'utilisateur dans ce navigateur.",
        "01-connexion.jpg",
        [
            ("Adresse", "Utiliser pstm.local ou l'IP locale fournie par l'installateur."),
            ("Identifiants", "Ne jamais partager le mot de passe. Demander une reinitialisation a l'administrateur si necessaire."),
            ("Deconnexion", "Toujours utiliser Deconnexion sur un poste partage."),
        ],
        "2 - Connexion",
    )

    story.extend(page_title("Premiere configuration du site", "3 - Administrateur - ordre recommande"))
    story.append(
        make_table(
            ["Etape", "Menu", "Action a terminer"],
            [
                ["1", "Mot de passe / Token", "Changer immediatement le mot de passe admin par defaut"],
                ["2", "Configuration", "Definir duree, valeur et solde ; regler le tarif dans Gestion des postes"],
                ["3", "Gestion des postes", "Attendre chaque decouverte, donner un nom clair, utiliser Ping"],
                ["4", "Utilisateurs", "Creer un compte nominatif par personne et choisir le bon role"],
                ["5", "Mot de passe / Token", "Regenerer le token API seulement si une integration externe est prevue"],
                ["6", "Configuration", "Exporter la sauvegarde JSON initiale et la stocker de facon protegee"],
                ["7", "Vue des postes", "Tester +N, arret, expiration, hors ligne et reprise"],
                ["8", "Recette + Journal", "Tester une piece reelle, les periodes, CSV/PDF et les messages"],
            ],
            [2.0 * cm, 7.0 * cm, 17.1 * cm],
        )
    )
    story.append(Spacer(1, 0.28 * cm))
    story.append(note_box("Compte initial", "Sur une centrale neuve, l'identifiant temporaire est admin et le mot de passe 12345678. Ne pas remettre le systeme au client tant que ce mot de passe n'a pas ete remplace.", "red"))
    story.append(Spacer(1, 0.22 * cm))
    story.append(note_box("Acces", "Le systeme est concu pour un LAN prive en HTTP. Aucun port du routeur ne doit etre redirige vers la centrale.", "amber"))
    story.append(PageBreak())

    add_screenshot_page(
        story,
        "Vue des postes - administrateur",
        "L'administrateur voit toutes les pages, le bouton de simulation de coin et la reinitialisation Wi-Fi. Les quatre cartes de poste montrent volontairement les principaux etats possibles.",
        "02-tableau-de-bord-admin.jpg",
        [
            ("Indicateurs", "Solde, duree par coin, actifs, hors ligne et reprises. Le bouton Masquer les cartes replie ces cinq indicateurs."),
            ("Sessions", "Le bouton +N applique le tarif du poste et ajoute une seule duree. Arreter coupe le relais et perd le temps restant apres confirmation."),
            ("Etats", "Vert : actif. Gris : inactif. Rouge : hors ligne. Jaune : reprise en attente."),
        ],
        "4 - Tableau de bord",
    )

    add_screenshot_page(
        story,
        "Vue des postes - utilisateur simple",
        "Le compte utilisateur simple garde les commandes d'exploitation quotidiennes, mais ne voit aucune page d'administration ni bouton de simulation de piece.",
        "03-tableau-de-bord-utilisateur.jpg",
        [
            ("Menu limite", "Seule la Vue des postes est disponible."),
            ("Temps", "Le bouton +N demarre une session ou ajoute une duree sur un poste actif, en debitant son tarif."),
            ("Reprise", "Reprendre relance le temps sauvegarde. Annuler l'efface definitivement apres confirmation."),
        ],
        "5 - Tableau de bord",
    )

    story.extend(page_title("Affecter du temps, arreter et traiter une coupure", "6 - Exploitation quotidienne"))
    story.append(
        make_table(
            ["Situation", "Action", "Effet"],
            [
                ["Poste inactif", "Cliquer +N pieces", "La centrale envoie START_SESSION et debite les coins apres reponse du poste"],
                ["Poste actif", "Cliquer +N pieces", "La centrale envoie ADD_TIME et prolonge l'heure de fin"],
                ["Arret demande", "Cliquer Arreter la session puis confirmer", "Le relais passe OFF et le temps restant est perdu"],
                ["Poste hors ligne", "Verifier alimentation et reseau", "Aucune nouvelle session ne peut etre envoyee"],
                ["Reprise en attente", "Choisir Reprendre", "Le relais redemarre avec le temps sauvegarde"],
                ["Reprise refusee", "Choisir Annuler puis confirmer", "Le snapshot NVS du poste est efface"],
            ],
            [5.0 * cm, 7.2 * cm, 13.9 * cm],
        )
    )
    story.append(Spacer(1, 0.3 * cm))
    story.append(note_box("Debit des coins", "Les coins sont debites uniquement quand le poste repond avec succes. En cas d'echec reseau, verifier l'etat du poste puis recommencer une seule fois.", "green"))
    story.append(Spacer(1, 0.3 * cm))
    story.append(note_box("Coupure electrique", "Le temps hors tension n'est pas deduit et le dernier checkpoint peut surestimer la duree d'environ 60 secondes. Le relais reste OFF jusqu'a la decision de l'operateur.", "amber"))
    story.append(PageBreak())

    add_screenshot_page(
        story,
        "Consulter et exporter la recette",
        "Page reservee aux administrateurs. Choisir Jour, Semaine ou Mois et une date de reference. Les exports CSV et PDF reprennent la periode affichee.",
        "04-recette.jpg",
        [
            ("Synthese", "Recette totale, impulsions, coins generes et nombre d'ecritures."),
            ("Detail", "Le graphique et le tableau utilisent les impulsions physiques datees."),
            ("Exclusions", "Les coins simules et les corrections manuelles du solde ne sont pas inclus dans la recette."),
        ],
        "7 - Administration",
    )

    add_screenshot_page(
        story,
        "Modifier la configuration et gerer les sauvegardes",
        "Cette page regroupe la duree par attribution, la valeur monetaire et le solde en pieces. Le nombre de pieces requis se regle par poste dans Gestion des postes. Elle permet aussi l'export et la restauration de la configuration complete.",
        "05-configuration.jpg",
        [
            ("Reglages", "Modifier une valeur, puis cliquer Enregistrer les modifications."),
            ("Export", "Telecharger une sauvegarde avant toute operation importante."),
            ("Restauration", "Le JSON remplace les comptes, postes et donnees reseau, puis impose une nouvelle connexion."),
        ],
        "8 - Administration",
    )

    add_screenshot_page(
        story,
        "Configurer et maintenir les postes",
        "Les postes connus apparaissent a gauche. Un nouveau module connecte au LAN apparait a droite avec son chipId et son IP, puis recoit un nom visible lors de l'ajout.",
        "06-gestion-des-postes.jpg",
        [
            ("Ajouter", "Cliquer Ajouter ce poste et saisir un nom simple comme Poste 5."),
            ("Verrouillage", "Un poste actif ou en reprise ne peut pas etre modifie ni supprime."),
            ("Ping", "Utiliser Ping pour confirmer la communication directe avant un test de session."),
        ],
        "9 - Administration",
    )

    add_screenshot_page(
        story,
        "Gerer les utilisateurs",
        "Creer un nom d'utilisateur unique, un prenom, un nom, un mot de passe et un role. Conserver au moins un administrateur et ne jamais supprimer son propre compte.",
        "07-utilisateurs.jpg",
        [
            ("Creation", "Choisir Utilisateur simple pour l'exploitation quotidienne."),
            ("Modification", "L'icone crayon permet de changer nom, role et mot de passe d'un autre compte."),
            ("Suppression", "L'icone corbeille est desactivee pour le compte actuellement connecte."),
        ],
        "10 - Administration",
    )

    add_screenshot_page(
        story,
        "Renforcer la securite",
        "Changer le mot de passe administrateur des la premiere mise en service. Le token API reste masque et ne doit etre regenere que si tous les clients autorises peuvent etre mis a jour.",
        "08-securite.jpg",
        [
            ("Mot de passe", "Utiliser une phrase longue et unique. La modification ferme la session et demande une reconnexion."),
            ("Token API", "Le token complet n'est affiche qu'a la regeneration. Le copier et le conserver dans un emplacement protege."),
            ("Regeneration", "L'ancien token devient immediatement invalide."),
        ],
        "11 - Administration",
    )

    add_screenshot_page(
        story,
        "Lire le journal d'evenements",
        "Le journal conserve les 100 derniers evenements utiles : demarrage, annonces, affectations, erreurs, pertes de communication, recette et sauvegarde.",
        "09-journal.jpg",
        [
            ("Niveaux", "INFO decrit une operation normale. AVERTISSEMENT signale un evenement a verifier. ERREUR indique un echec."),
            ("Actualiser", "La page se recharge automatiquement et peut aussi etre actualisee manuellement."),
            ("Vider", "La suppression est definitive et demande une confirmation. La reserver au diagnostic termine."),
        ],
        "12 - Administration",
    )

    add_screenshot_page(
        story,
        "Reconnecter la centrale au Wi-Fi",
        "Apres Reinitialiser le Wi-Fi, se connecter au point d'acces GAMEROOM-CENTRAL-&lt;chipId&gt;, puis ouvrir le portail ou http://192.168.4.1.",
        "10-wifi-centrale.jpg",
        [
            ("SSID", "Les noms de reseau de la capture sont des exemples masques."),
            ("DHCP", "Conserver Automatique sauf consigne precise de l'installateur reseau."),
            ("Apres succes", "Reconnecter le telephone ou l'ordinateur au LAN, puis rouvrir pstm.local."),
        ],
        "13 - Reseau",
    )

    add_screenshot_page(
        story,
        "Reconnecter un poste au Wi-Fi",
        "Un poste sans identifiants valides ouvre GAMEROOM-POSTE-&lt;chipId&gt;. Choisir le meme reseau local que la centrale. Il reapparait automatiquement dans Gestion des postes.",
        "11-wifi-poste.jpg",
        [
            ("Meme LAN", "La centrale et les postes doivent communiquer directement entre clients."),
            ("Nom du poste", "La configuration Wi-Fi ne change pas le nom logique deja enregistre en NVS."),
            ("Verification", "Attendre l'annonce, puis utiliser Ping et effectuer une session de test."),
        ],
        "14 - Reseau",
    )

    story.extend(page_title("Sauvegarder, restaurer et proteger les donnees", "15 - Bonnes pratiques"))
    story.append(
        make_table(
            ["Action", "Quand", "Precaution"],
            [
                ["Exporter JSON", "Avant mise a jour, intervention ou changement important", "Stocker chiffre et limiter l'acces"],
                ["Restaurer JSON", "Apres perte ou remplacement de centrale", "Verifier la provenance et accepter la reconnexion"],
                ["Exporter recette", "Selon la frequence comptable", "Comparer la periode et la date de reference"],
                ["Changer mot de passe", "Premiere installation et suspicion de fuite", "Utiliser une valeur unique"],
                ["Regenerer token", "Uniquement si compromis ou renouvellement planifie", "Mettre a jour tous les clients API"],
                ["Reinitialiser Wi-Fi", "Changement de routeur ou de SSID", "Prevoir un acces physique a la centrale"],
            ],
            [6.2 * cm, 9.1 * cm, 10.8 * cm],
        )
    )
    story.append(Spacer(1, 0.3 * cm))
    story.append(note_box("Fichier sensible", "La sauvegarde peut contenir les comptes, le token API, les postes et les parametres Wi-Fi/IP. Ne jamais l'envoyer par messagerie non securisee ni la laisser sur un poste public.", "red"))
    story.append(PageBreak())

    story.extend(page_title("Depannage rapide", "16 - Incidents courants"))
    story.append(
        make_table(
            ["Symptome", "Verification", "Action"],
            [
                ["pstm.local ne repond pas", "Meme Wi-Fi, isolation des clients, IP centrale", "Ouvrir l'IP directe puis corriger mDNS ou le LAN"],
                ["Poste hors ligne", "Alimentation, Wi-Fi, IP, Ping et token partage", "Redemarrer le poste et consulter le journal"],
                ["Relais inverse", "Logique du module", "Faire corriger RELAY_ACTIVE_HIGH par le technicien"],
                ["TM1637 eteint", "3,3 V, GND, CLK GPIO25, DIO GPIO26", "Couper l'alimentation et verifier le cablage"],
                ["Coin non compte", "GPIO13, pull-down 4,7 kOhm, masse commune, impulsions et alimentation monnayeur", "Mesurer COIN puis verifier le cablage et la configuration"],
                ["Recette absente", "LittleFS pret et horloge NTP synchronisee", "Verifier le journal et le reseau Internet NTP"],
                ["Reprise inattendue", "Coupure pendant une session et checkpoint NVS", "Choisir Reprendre ou Annuler pour ce poste"],
                ["Connexion refusee", "Nom en minuscules et mot de passe", "Contacter un administrateur, ne pas multiplier les essais"],
            ],
            [6.4 * cm, 10.0 * cm, 9.7 * cm],
        )
    )
    story.append(Spacer(1, 0.28 * cm))
    story.append(note_box("Avant d'appeler le support", "Noter l'heure, le poste concerne, son etat, l'IP affichee et le dernier message du journal. Ne jamais transmettre de mot de passe, token complet ou sauvegarde sans canal protege.", "blue"))
    story.append(PageBreak())

    story.extend(page_title("Routine d'ouverture et de fermeture", "17 - Exploitation du site"))
    story.append(
        make_table(
            ["Moment", "Controle", "Action si anomalie"],
            [
                ["Ouverture - 1", "Ouvrir pstm.local, se connecter et verifier la date/heure", "Utiliser l'IP directe ; signaler une horloge non synchronisee"],
                ["Ouverture - 2", "Verifier solde, postes hors ligne et reprises en attente", "Traiter chaque reprise ; controler alimentation et Ping"],
                ["Ouverture - 3", "Tester une session courte sur un poste temoin", "Ne pas ouvrir au public si relais ou decompte incorrect"],
                ["Pendant service", "Affecter les coins uniquement au poste demande ; confirmer avant Arreter", "Consulter Journal en cas d'echec ; ne pas repeter sans controle"],
                ["Fermeture - 1", "Verifier qu'aucune session active ou reprise n'est oubliee", "Contacter le responsable avant toute annulation de temps"],
                ["Fermeture - 2", "Exporter la recette selon la frequence comptable", "Verifier periode et date de reference avant archivage"],
                ["Fermeture - 3", "Examiner les avertissements/erreurs, puis se deconnecter", "Noter heure, poste, IP et message pour le support"],
                ["Periodique", "Exporter la configuration JSON et tester la procedure de restauration", "Conserver le fichier chiffre et hors du poste public"],
            ],
            [4.2 * cm, 13.0 * cm, 8.9 * cm],
        )
    )
    story.append(Spacer(1, 0.28 * cm))
    story.append(note_box("Restauration", "Une restauration remplace comptes, postes et parametres reseau, puis impose une reconnexion. La recette LittleFS n'est pas contenue dans l'export JSON. Effectuer cette operation hors horaires d'exploitation et avec une sauvegarde valide.", "red"))
    story.append(PageBreak())

    story.extend(page_title("Fiche reflexe pour l'operateur", "18 - Resume"))
    daily = [
        ["Ouvrir", "pstm.local, se connecter, verifier solde et postes hors ligne"],
        ["Demarrer", "Sur le poste choisi, cliquer sur le bouton +N"],
        ["Prolonger", "Sur un poste actif, cliquer sur le bouton +N"],
        ["Arreter", "Cliquer Arreter la session et confirmer la perte du temps restant"],
        ["Coupure", "Choisir Reprendre ou Annuler uniquement pour le poste concerne"],
        ["Fin de service", "Verifier les alertes, exporter la recette si prevu et se deconnecter"],
    ]
    story.append(make_table(["Moment", "Action"], daily, [5.2 * cm, 20.9 * cm]))
    story.append(Spacer(1, 0.35 * cm))
    story.append(
        note_box(
            "Priorite en cas de doute",
            "Ne pas ouvrir les coffrets ni intervenir sur le cablage. Mettre la zone en securite, noter le symptome et contacter l'installateur ou le responsable technique.",
            "amber",
        )
    )
    story.append(PageBreak())

    story.extend(page_title("Remise du systeme a un nouveau client", "19 - Checklist de deploiement"))
    handover = [
        ["Acces", "URL/IP locale documentee ; mot de passe admin par defaut remplace", "OK / NOK"],
        ["Roles", "Comptes nominatifs crees ; utilisateur simple teste sans menu admin", "OK / NOK"],
        ["Postes", "Noms conformes au plan de salle ; Ping, +N, arret et expiration testes", "OK / NOK"],
        ["Reprise", "Reprendre et Annuler expliques sur une coupure de demonstration", "OK / NOK"],
        ["Recette", "Jour/semaine/mois et exports CSV/PDF expliques", "OK / NOK"],
        ["Sauvegarde", "Export JSON initial protege ; responsable et emplacement consignes", "OK / NOK"],
        ["Reseau", "SSID reel non imprime dans ce manuel ; IP centrale conservee separement", "OK / NOK"],
        ["Securite", "Aucune redirection Internet ; token API remis uniquement si necessaire", "OK / NOK"],
        ["Support", "Procedure d'incident et coordonnees de l'installateur communiquees", "OK / NOK"],
        ["Formation", "Administrateur et operateur ont effectue un parcours complet", "OK / NOK"],
    ]
    story.append(make_table(["Lot", "Critere de remise", "Resultat"], handover, [4.2 * cm, 18.6 * cm, 3.3 * cm]))
    story.append(Spacer(1, 0.25 * cm))
    story.append(note_box("Document reutilisable", "Conserver ce manuel generique sans donnees de site. Remettre les IP, identifiants initiaux, plan de salle et contacts dans une fiche de deploiement separee et protegee.", "green"))
    return story


def build_pdf(path: Path, label: str, title: str, subject: str, story: list[Flowable]) -> None:
    doc = ManualDocTemplate(
        str(path),
        document_label=label,
        leftMargin=MARGIN_X,
        rightMargin=MARGIN_X,
        topMargin=MARGIN_TOP,
        bottomMargin=MARGIN_BOTTOM,
        title=title,
        author="PS TIME MANAGER",
        subject=subject,
        creator="PS TIME MANAGER documentation generator",
    )
    doc.build(story)


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    technical = OUT / "ps-time-manager-guide-technique-client.pdf"
    user = OUT / "ps-time-manager-manuel-utilisation.pdf"
    build_pdf(
        technical,
        "Guide technique client",
        "PS TIME MANAGER - Guide technique d'installation et de mise en service",
        "Installation des outils, cablage, compilation, reseau, tests et maintenance",
        build_technical_story(),
    )
    build_pdf(
        user,
        "Manuel d'utilisation",
        "PS TIME MANAGER - Manuel d'utilisation administrateur et utilisateur simple",
        "Exploitation quotidienne et administration de PS TIME MANAGER",
        build_user_story(),
    )
    print(technical)
    print(user)


if __name__ == "__main__":
    main()
