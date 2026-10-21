#!/usr/bin/env python3
"""Construit les deux PDF depuis leurs sources Markdown et les captures réelles.

Dépendances : ReportLab, Pillow et pypdf. Les images sont intégrées sans retouche et
les captures longues sont réparties en bandes successives, sans déformation.
"""
from __future__ import annotations

import html
import hashlib
import json
import math
import re
from pathlib import Path
from urllib.parse import quote

import reportlab
from PIL import Image as PILImage, ImageChops
from pypdf import PdfReader, PdfWriter
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.utils import ImageReader
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (
    BaseDocTemplate, Flowable, Frame, KeepTogether, PageBreak, PageTemplate,
    Paragraph, Spacer, Table, TableStyle,
)

ROOT = Path(__file__).resolve().parents[2]
SOURCES = ROOT / "docs/manuals"
OUT = ROOT / "output/pdf"
WIDTH, HEIGHT = A4
MARGIN = 42
CONTENT_WIDTH = WIDTH - 2 * MARGIN
NAVY = colors.HexColor("#0a192e")
BLUE = colors.HexColor("#1469cc")
MUTED = colors.HexColor("#5c6d80")
PALE = colors.HexColor("#edf3fa")

# Les polices Vera sont livrées avec ReportLab : aucun chemin propre à un poste.
FONT_DIR = Path(reportlab.__file__).parent / "fonts"
for name, filename in [("Body", "Vera.ttf"), ("Body-Bold", "VeraBd.ttf"), ("Body-Italic", "VeraIt.ttf")]:
    pdfmetrics.registerFont(TTFont(name, str(FONT_DIR / filename)))
pdfmetrics.registerFontFamily("Body", normal="Body", bold="Body-Bold", italic="Body-Italic", boldItalic="Body-Bold")
styles = getSampleStyleSheet()
styles.add(ParagraphStyle("BodyTextFR", fontName="Body", fontSize=9.5, leading=14, spaceAfter=8, textColor=NAVY, allowWidows=0, allowOrphans=0))
styles.add(ParagraphStyle("StepFR", parent=styles["BodyTextFR"], leftIndent=20, bulletIndent=0, bulletFontName="Body-Bold", bulletFontSize=9.5, spaceAfter=6))
styles.add(ParagraphStyle("H1FR", fontName="Body-Bold", fontSize=20, leading=25, spaceAfter=13, textColor=NAVY, keepWithNext=True))
styles.add(ParagraphStyle("H2FR", fontName="Body-Bold", fontSize=12, leading=17, spaceBefore=10, spaceAfter=8, textColor=BLUE, keepWithNext=True))
styles.add(ParagraphStyle("CellFR", fontName="Body", fontSize=8, leading=11, textColor=NAVY, wordWrap="CJK"))
styles.add(ParagraphStyle("HeadFR", parent=styles["CellFR"], fontName="Body-Bold", textColor=colors.white))
styles.add(ParagraphStyle("CaptionFR", fontName="Body-Italic", fontSize=8, leading=11.5, textColor=MUTED, spaceAfter=12))
styles.add(ParagraphStyle("CodeFR", fontName="Courier", fontSize=7.4, leading=10.3, textColor=NAVY, wordWrap="CJK"))
styles.add(ParagraphStyle("TocFR", fontName="Body", fontSize=9, leading=13, spaceAfter=4, textColor=BLUE))
styles.add(ParagraphStyle("CoverFR", fontName="Body-Bold", fontSize=31, leading=39, textColor=NAVY, spaceAfter=22))


def normalize(text: str) -> str:
    for a, b in {"–":"-", "—":"-", "‑":"-", "→":" > ", "\u202f":" ", "\u00a0":" "}.items():
        text = text.replace(a, b)
    return text


def inline(text: str, source: Path) -> str:
    # Échapper d'abord, puis construire seulement les balises autorisées.
    text = html.escape(normalize(text), quote=False)
    text = re.sub(r"`([^`]+)`", r'<font name="Courier">\1</font>', text)
    text = re.sub(r"\*\*([^*]+)\*\*", r"<b>\1</b>", text)
    def link(match):
        label, target = match.groups()
        if not re.match(r"https?://|mailto:|#", target):
            absolute = (source.parent / html.unescape(target)).resolve()
            if not absolute.exists():
                raise FileNotFoundError(f"Lien introuvable : {source.name}: {target}")
            from os.path import relpath
            sibling_pdfs = {"guide-technique.md":"ps-time-manager-guide-technique-client.pdf", "manuel-utilisation.md":"ps-time-manager-manuel-utilisation.pdf"}
            target = sibling_pdfs.get(absolute.name, quote(relpath(absolute, OUT), safe="/#:"))
        return f'<link href="{html.escape(target, quote=True)}" color="#1469cc">{label}</link>'
    return re.sub(r"\[([^\]]+)\]\(([^)]+)\)", link, text)


def paragraph(text: str, source: Path, style="BodyTextFR") -> Paragraph:
    return Paragraph(inline(text, source), styles[style])


class ImageBand(Flowable):
    """Bande d'une capture, affichée par clipping PDF du fichier original."""
    def __init__(self, path: Path, width: float, full_height: float, offset: float, height: float):
        super().__init__()
        self.path, self.width, self.full_height = path, width, full_height
        self.offset, self.height = offset, height
    def draw(self):
        c = self.canv
        c.saveState()
        clip = c.beginPath()
        clip.rect(0, 0, self.width, self.height)
        c.clipPath(clip, stroke=0, fill=0)
        c.drawImage(ImageReader(str(self.path)), 0, self.height-self.full_height+self.offset,
                    width=self.width, height=self.full_height, mask="auto")
        c.restoreState()
        c.setStrokeColor(colors.HexColor("#c9d6e4"))
        c.rect(0, 0, self.width, self.height, stroke=1, fill=0)


class Doc(BaseDocTemplate):
    def __init__(self, filename, label, **kwargs):
        super().__init__(filename, pagesize=A4, leftMargin=MARGIN, rightMargin=MARGIN,
                         topMargin=49, bottomMargin=43, title=f"PS TIME MANAGER - {label}",
                         author="PS TIME MANAGER", subject="Installation et utilisation de la centrale et des postes ESP32", **kwargs)
        self.label = label
        self.addPageTemplates(PageTemplate(id="main", frames=[Frame(MARGIN,43,CONTENT_WIDTH,HEIGHT-92, leftPadding=0,rightPadding=0,topPadding=0,bottomPadding=0)], onPage=self.frame))
        self.outline_index = 0
    def frame(self, canvas, doc):
        canvas.saveState()
        canvas.setFillColor(NAVY)
        canvas.rect(0, HEIGHT-28, WIDTH, 28, fill=1, stroke=0)
        canvas.setFillColor(colors.white)
        canvas.setFont("Body-Bold", 8)
        canvas.drawString(MARGIN, HEIGHT-18, "PS TIME MANAGER")
        canvas.setFont("Body", 7.5)
        canvas.drawRightString(WIDTH-MARGIN, HEIGHT-18, self.label)
        canvas.setFillColor(MUTED)
        canvas.setFont("Body", 7)
        canvas.drawString(MARGIN, 23, "Centrale et postes ESP32")
        canvas.drawRightString(WIDTH-MARGIN,23, str(doc.page))
        canvas.restoreState()
    def afterFlowable(self, flowable):
        if getattr(flowable, "section_title", None):
            key = f"section-{self.outline_index}"
            self.outline_index += 1
            self.canv.bookmarkPage(key)
            self.canv.addOutlineEntry(flowable.section_title, key, level=0, closed=False)


def table(lines: list[str], source: Path) -> Table:
    rows = [[cell.strip() for cell in line.strip().strip("|").split("|")] for line in lines]
    rows = [row for row in rows if not all(re.fullmatch(r":?-+:?", cell) for cell in row)]
    columns = len(rows[0])
    if any(len(row) != columns for row in rows):
        raise ValueError(f"Table irrégulière dans {source}")
    weights = [max(len(re.sub(r"[`*]", "", row[i])) for row in rows) ** .5 for i in range(columns)]
    widths = [CONTENT_WIDTH * w / sum(weights) for w in weights]
    data = [[paragraph(cell, source, "HeadFR" if r == 0 else "CellFR") for cell in row] for r,row in enumerate(rows)]
    result = Table(data, colWidths=widths, repeatRows=1, hAlign="LEFT")
    result.setStyle(TableStyle([
        ("BACKGROUND",(0,0),(-1,0),NAVY), ("VALIGN",(0,0),(-1,-1),"TOP"),
        ("ROWBACKGROUNDS",(0,1),(-1,-1),[colors.white,PALE]),
        ("GRID",(0,0),(-1,-1),.4,colors.HexColor("#d0dbe7")),
        ("LEFTPADDING",(0,0),(-1,-1),7), ("RIGHTPADDING",(0,0),(-1,-1),7),
        ("TOPPADDING",(0,0),(-1,-1),6), ("BOTTOMPADDING",(0,0),(-1,-1),6),
    ]))
    return result


def story_for(source: Path) -> list:
    lines = source.read_text(encoding="utf-8").splitlines()
    title = lines[0].lstrip("# ")
    first_section = next(i for i,line in enumerate(lines) if line.startswith("## "))
    introduction = " ".join(" ".join(lines[1:first_section]).split())
    story = [Spacer(1,90), paragraph("PS TIME MANAGER",source,"H2FR"),
             paragraph(title,source,"CoverFR"),paragraph("Centrale et postes ESP32",source,"H1FR"),
             Spacer(1,25),
             paragraph("Tarification par poste - Recettes TND/DZD - Sauvegardes V4",source),
             Spacer(1,22),paragraph(introduction,source),PageBreak()]
    headings=[line[3:] for line in lines if line.startswith("## ")]
    toc=Paragraph("Sommaire",styles["H1FR"]);toc.section_title="Sommaire"
    story += [toc,paragraph("Les signets du PDF donnent accès aux sections. Les captures longues se poursuivent sur la page suivante, sans réduction excessive.",source)]
    for number, heading in enumerate(headings, start=1):
        label = html.escape(normalize(heading))
        story.append(Paragraph(f'<link href="#section-{number}">{label}</link>', styles["TocFR"]))
    story.append(PageBreak())
    i=first_section
    while i<len(lines):
        line=lines[i].strip()
        if not line:
            i+=1;continue
        if line.startswith("## "):
            if not isinstance(story[-1],PageBreak): story.append(PageBreak())
            heading=paragraph(line[3:],source,"H1FR");heading.section_title=normalize(line[3:])
            story.append(heading);i+=1;continue
        if line.startswith("### "):
            story.append(paragraph(line[4:],source,"H2FR"));i+=1;continue
        numbered=re.match(r"^(\d+)\.\s+(.+)$",line)
        if numbered:
            story.append(Paragraph(inline(numbered[2],source),styles["StepFR"],bulletText=f"{numbered[1]}."))
            i+=1;continue
        if line.startswith("- "):
            story.append(Paragraph(inline(line[2:],source),styles["StepFR"],bulletText="-"))
            i+=1;continue
        if line.startswith("```"):
            code=[];i+=1
            while i<len(lines) and not lines[i].startswith("```"):
                # Le retour visuel conserve les caractères de chaque commande.
                text=normalize(lines[i]); code.extend([text[j:j+99] for j in range(0,len(text),99)] or [""])
                i+=1
            code_rows=[[Paragraph(html.escape(value).replace(" ","&#160;"),styles["CodeFR"])] for value in code]
            box=Table(code_rows,colWidths=[CONTENT_WIDTH])
            box.setStyle(TableStyle([("BACKGROUND",(0,0),(-1,-1),PALE),("LEFTPADDING",(0,0),(-1,-1),10),("TOPPADDING",(0,0),(-1,-1),2),("BOTTOMPADDING",(0,0),(-1,-1),2)]))
            story += [box,Spacer(1,10)];i+=1;continue
        if line.startswith("|"):
            group=[]
            while i<len(lines) and lines[i].strip().startswith("|"):
                group.append(lines[i]);i+=1
            story += [table(group,source),Spacer(1,12)];continue
        match=re.fullmatch(r"!\[([^\]]*)\]\(([^)]+)\)",line)
        if match:
            caption, relative=match.groups();path=(source.parent/relative).resolve()
            with PILImage.open(path) as im: w,h=im.size
            width=min(CONTENT_WIDTH,260 if w<700 else CONTENT_WIDTH)
            full_height=h*width/w
            # Très longue capture : largeur de lecture inchangée, bandes de 590 pt maximum.
            count=max(1,math.ceil(full_height/590))
            cuts=[0]
            # Placer les séparations entre les lignes visuelles plutôt qu'au
            # milieu d'un libellé ou d'une ligne de tableau. Le bitmap reste intact.
            with PILImage.open(path) as original:
                gray=original.convert("L")
                for boundary in range(1,count):
                    target=round(h*boundary/count)
                    def edge_score(y):
                        band=gray.crop((0,y-2,w,y+3))
                        edges=ImageChops.difference(band.crop((1,0,w,5)),band.crop((0,0,w-1,5)))
                        return sum(level*amount for level,amount in enumerate(edges.histogram())) + abs(y-target)*.01
                    cuts.append(min(range(max(3,target-32),min(h-3,target+33)),key=edge_score))
            cuts.append(h)
            for part in range(count):
                if part: story.append(PageBreak())
                image=ImageBand(path,width,full_height,cuts[part]*width/w,(cuts[part+1]-cuts[part])*width/w)
                image.hAlign="CENTER"
                label=caption+(f" (partie {part+1}/{count})" if count>1 else "")
                story.append(KeepTogether([image,Spacer(1,7),paragraph(label,source,"CaptionFR")]))
            i+=1;continue
        block=[line];i+=1
        while i<len(lines) and lines[i].strip() and not re.match(r"^(#{1,3} |!\[|\||```|\d+\.\s|- )",lines[i]):
            block.append(lines[i].strip());i+=1
        story.append(paragraph(" ".join(block),source))
    return story


def verify_captures():
    manifest = json.loads((SOURCES / "captures.json").read_text(encoding="utf-8"))
    expected = dict(manifest["uiSourceHashes"])
    expected.update({item["file"]: item["sha256"] for item in manifest["captures"]})
    for relative, digest in expected.items():
        path = ROOT / relative
        if not path.exists() or hashlib.sha256(path.read_bytes()).hexdigest() != digest:
            raise ValueError(f"Provenance modifiée : {relative}. Renouveler les captures et leur manifeste.")


def main():
    verify_captures()
    OUT.mkdir(parents=True,exist_ok=True)
    outputs=[("guide-technique.md","ps-time-manager-guide-technique-client.pdf","Guide technique client"),
             ("manuel-utilisation.md","ps-time-manager-manuel-utilisation.pdf","Manuel d’utilisation")]
    for filename,output,label in outputs:
        source=SOURCES/filename
        Doc(str(OUT/output),label).build(story_for(source))
        # Ne pas réintroduire de date éditoriale dans les métadonnées PDF.
        reader=PdfReader(OUT/output)
        writer=PdfWriter(clone_from=reader)
        writer.metadata={key:value for key,value in (reader.metadata or {}).items()
                         if key not in {"/CreationDate","/ModDate"}}
        temporary=OUT/(output+".tmp")
        with temporary.open("wb") as stream:
            writer.write(stream)
        reader.close()
        temporary.replace(OUT/output)
        print(OUT/output)


if __name__=="__main__": main()
