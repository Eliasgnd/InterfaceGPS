import os
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent
EXCLUDE_DIRS = {"build", ".git", ".vs", "Debug", "Release"}

# class/struct + héritage (comme avant)
CLASS_RE = re.compile(r'^\s*(class|struct)\s+([A-Za-z_]\w*)\s*(?:\:\s*([^{]+))?\s*\{', re.MULTILINE)

# Définitions dans cpp: ReturnType Class::method(args) { ... }
CPP_METHOD_RE = re.compile(r'^\s*(?:[\w:\<\>\&\*\s]+?\s+)?([A-Za-z_]\w*)::([A-Za-z_]\w*)\s*\(([^;{}]*)\)\s*(?:const)?\s*\{', re.MULTILINE)

def should_skip_dir(p: Path) -> bool:
    parts = {x.lower() for x in p.parts}
    return any(d.lower() in parts for d in EXCLUDE_DIRS)

def strip_comments(code: str) -> str:
    code = re.sub(r'//.*?$', '', code, flags=re.MULTILINE)
    code = re.sub(r'/\*.*?\*/', '', code, flags=re.DOTALL)
    return code

def extract_class_block(text: str, start: int):
    i = text.find("{", start)
    if i == -1:
        return None
    depth = 0
    for j in range(i, len(text)):
        if text[j] == "{": depth += 1
        elif text[j] == "}":
            depth -= 1
            if depth == 0:
                return text[i+1:j]
    return None

def clean_inheritance(inh: str):
    if not inh:
        return []
    out = []
    for chunk in inh.split(","):
        chunk = re.sub(r'\b(public|protected|private|virtual)\b', '', chunk).strip()
        chunk = re.sub(r'<.*?>', '', chunk).strip()
        m = re.search(r'([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)$', chunk)
        if m: out.append(m.group(1))
    return out

def normalize_decl(s: str) -> str:
    s = s.strip().rstrip(";").strip()
    s = re.sub(r'\b(Q_INVOKABLE|Q_SLOT|Q_SIGNAL)\b', '', s)
    s = re.sub(r'\s+', ' ', s)
    return s

def parse_header_members(block: str):
    """
    Extrait surtout: signals / slots / public méthodes (déclarations) depuis le header.
    """
    block = strip_comments(block)

    section = "private"
    methods_pub, fields_pub, sigs, sls = [], [], [], []

    # On split par ';' pour attraper les déclarations
    parts = [p.strip() for p in block.replace("\n", " ").split(";") if p.strip()]

    for p in parts:
        # repérer changements de section (même si collé)
        # ex: "public: ..." ou "signals: ..."
        if re.search(r'\bpublic\s*:\b', p): section = "public"
        if re.search(r'\bprotected\s*:\b', p): section = "protected"
        if re.search(r'\bprivate\s*:\b', p): section = "private"
        if re.search(r'\bsignals\s*:\b', p): section = "signals"
        if re.search(r'\b(public|protected|private)?\s*slots\s*:\b', p): section = "slots"

        # ignore macros
        if "Q_OBJECT" in p or p.strip().startswith("Q_PROPERTY"):
            continue

        # méthode déclarée ?
        if "(" in p and ")" in p and "typedef" not in p and "using" not in p:
            decl = normalize_decl(p)
            # évite les constructeurs vides genre "public:"
            if "::" in decl:  # ne devrait pas être dans header
                continue
            # enlever "public:" etc si présent au début
            decl = re.sub(r'^(public|protected|private|signals|slots)\s*:\s*', '', decl).strip()
            if not decl:
                continue
            if section == "signals":
                sigs.append(decl)
            elif section == "slots":
                sls.append(decl)
            elif section == "public":
                methods_pub.append(decl)
            continue

        # champ public (rare dans Qt mais au cas où)
        if section == "public":
            fields_pub.append(normalize_decl(p))

    # limiter
    def limit(lst, n=15):
        return lst[:n] + (["..."] if len(lst) > n else [])

    return limit(methods_pub), limit(fields_pub), limit(sigs), limit(sls)

def scan():
    classes = {}  # name -> info

    # 1) Parse headers pour classes + members
    for root, dirs, files in os.walk(ROOT):
        rp = Path(root)
        if should_skip_dir(rp):
            dirs[:] = []
            continue
        dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS]

        for fn in files:
            if not fn.lower().endswith((".h", ".hpp", ".hh", ".hxx")):
                continue
            path = rp / fn
            text = path.read_text(encoding="utf-8", errors="ignore")

            for m in CLASS_RE.finditer(text):
                kind, name, inh = m.group(1), m.group(2), m.group(3) or ""
                block = extract_class_block(text, m.start())
                if not block:
                    continue
                methods_pub, fields_pub, sigs, sls = parse_header_members(block)
                classes[name] = {
                    "kind": kind,
                    "bases": clean_inheritance(inh),
                    "methods": methods_pub,
                    "fields": fields_pub,
                    "signals": sigs,
                    "slots": sls,
                    "cpp_methods": []  # rempli ensuite
                }

    # 2) Parse cpp pour méthodes définies (Class::method)
    for root, dirs, files in os.walk(ROOT):
        rp = Path(root)
        if should_skip_dir(rp):
            dirs[:] = []
            continue
        dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS]

        for fn in files:
            if not fn.lower().endswith((".cpp", ".cc", ".cxx")):
                continue
            path = rp / fn
            text = path.read_text(encoding="utf-8", errors="ignore")

            for cls, meth, args in CPP_METHOD_RE.findall(text):
                if cls in classes:
                    sig = f"{meth}({re.sub(r'\\s+', ' ', args.strip())})"
                    if sig not in classes[cls]["cpp_methods"]:
                        classes[cls]["cpp_methods"].append(sig)

    # limiter cpp_methods
    for c in classes.values():
        c["cpp_methods"] = c["cpp_methods"][:20] + (["..."] if len(c["cpp_methods"]) > 20 else [])

    return classes

def gen_puml(classes: dict) -> str:
    lines = []
    lines.append("@startuml")
    lines.append("left to right direction")
    lines.append("skinparam classAttributeIconSize 0")
    lines.append("skinparam shadowing false")

    for name in sorted(classes.keys()):
        info = classes[name]
        lines.append(f"class {name} {{")
        for f in info["fields"]:
            lines.append(f"  + {f}")
        for me in info["methods"]:
            lines.append(f"  + {me}")
        if info["signals"]:
            lines.append("  .. signals ..")
            for s in info["signals"]:
                lines.append(f"  + {s}")
        if info["slots"]:
            lines.append("  .. slots ..")
            for s in info["slots"]:
                lines.append(f"  + {s}")
        if info["cpp_methods"]:
            lines.append("  .. defined in cpp ..")
            for s in info["cpp_methods"]:
                lines.append(f"  # {s}")
        lines.append("}")

    for name, info in classes.items():
        for base in info["bases"]:
            if base not in classes:
                lines.append(f"class {base} <<external>>")
            lines.append(f"{base} <|-- {name}")

    lines.append("@enduml")
    return "\n".join(lines)

def main():
    classes = scan()
    out = gen_puml(classes)
    out_path = ROOT / "classes_full_v3.puml"
    out_path.write_text(out, encoding="utf-8")
    print(f"OK: {len(classes)} classes trouvées")
    print(f"Généré: {out_path}")

if __name__ == "__main__":
    main()