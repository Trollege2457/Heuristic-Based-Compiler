from flask import Flask, request, render_template, jsonify
import subprocess
import os
import uuid
from graphviz import Digraph
import re

app = Flask(__name__)

BASE_DIR = "/root/ambiguity_parser"
GRAMMAR_PATH = os.path.join(BASE_DIR, "grammar.txt")


# ---------------- OPERAND NORMALIZATION ----------------
def normalize_operand(token):
    if token.isdigit():
        return "NUM"
    return "ID"


def extract_operands(expr):
    raw = re.findall(r'[a-zA-Z]+|[0-9]+', expr)
    return [normalize_operand(x) for x in raw]


# ---------------- TREE DRAW ----------------
def draw_tree(tree, filename):
    dot = Digraph()
    counter = [0]

    def add(node):
        if not node:
            return None

        node_id = str(counter[0])
        counter[0] += 1

        label = node[0] if isinstance(node, tuple) else str(node)
        dot.node(node_id, label)

        if isinstance(node, tuple) and len(node) > 1:
            l = add(node[1])
            r = add(node[2])

            if l:
                dot.edge(node_id, l)
            if r:
                dot.edge(node_id, r)

        return node_id

    add(tree)

    path = f"static/{filename}"
    dot.render(path, format="png", cleanup=True)
    return path + ".png"


# ---------------- SAFE TREE HELPERS ----------------
def safe_operands(ops, operands):
    
    if len(operands) < len(ops) + 1:
        return ["ID"] * (len(ops) + 1)
    return operands


# ---------------- TREE BUILDERS ----------------
def left_assoc_tree(op, operands):
    operands = safe_operands([op]*(len(operands)-1), operands)

    tree = (op, (operands[0],), (operands[1],))
    for i in range(2, len(operands)):
        tree = (op, tree, (operands[i],))
    return tree


def right_assoc_tree(op, operands):
    operands = safe_operands([op]*(len(operands)-1), operands)

    def build(i):
        if i == len(operands) - 1:
            return (operands[i],)
        return (op, (operands[i],), build(i + 1))

    return build(0)


def build_prec_tree(ops, operands):
    operands = safe_operands(ops, operands)

    nodes = [operands[0]]

    for i in range(len(ops)):
        nodes.append(ops[i])
        nodes.append(operands[i + 1])

    # high precedence
    i = 1
    while i < len(nodes) - 1:
        if nodes[i] in ["*", "/", "%"]:
            nodes[i - 1:i + 2] = [(nodes[i], nodes[i - 1], nodes[i + 1])]
            i = 1
        else:
            i += 2

    # low precedence
    i = 1
    while i < len(nodes) - 1:
        if nodes[i] in ["+", "-"]:
            nodes[i - 1:i + 2] = [(nodes[i], nodes[i - 1], nodes[i + 1])]
            i = 1
        else:
            i += 2

    return nodes[0]


def dangling_trees():
    t1 = ("IF", ("COND",),
          ("IF", ("COND",),
           ("ELSE", ("S1",), ("S2",))))

    t2 = ("IF", ("COND",),
          ("ELSE",
           ("IF", ("COND",), ("S1",)),
           ("S2",)))

    return t1, t2


# ---------------- ROUTES ----------------
@app.route("/")
def home():
    return render_template("index.html")


@app.route("/run", methods=["POST"])
def run():
    choice = request.form.get("choice")
    expr = request.form.get("expr", "").strip()

    img1 = None
    img2 = None

    if os.path.exists(GRAMMAR_PATH):
        os.remove(GRAMMAR_PATH)

    grammar_uploaded = False
    if "grammar" in request.files:
        file = request.files["grammar"]
        if file and file.filename != "":
            file.save(GRAMMAR_PATH)
            grammar_uploaded = True

    # ================= GRAMMAR =================
    if choice == "1":
        if not grammar_uploaded:
            return jsonify({"text": "ERROR: Upload grammar.txt", "img1": None, "img2": None})

        result = subprocess.run(
            ["./parser"],
            input="1\n",
            text=True,
            capture_output=True,
            cwd=BASE_DIR
        )

        return jsonify({
            "text": result.stdout.strip(),
            "img1": None,
            "img2": None
        })

    # ================= EXPRESSION =================
    elif choice == "2":

        if expr == "":
            return jsonify({"text": "Empty expression", "img1": None, "img2": None})

        result = subprocess.run(
            ["./parser"],
            input="2\n" + expr + "\n",
            text=True,
            capture_output=True,
            cwd=BASE_DIR
        )

        output = result.stdout.strip()

        ops = [c for c in expr if c in "+-*/%"]
        operands = extract_operands(expr)

        # SAFETY CHECK (CRITICAL FIX)
        if len(operands) < len(ops) + 1:
            return jsonify({
                "text": "INVALID\nIncorrect Witness String",
                "img1": None,
                "img2": None
            })

        # INVALID
        if "INVALID" in output:
            return jsonify({"text": output, "img1": None, "img2": None})

        # ASSOCIATIVITY
        if "Associativity" in output and len(ops) >= 2:
            op = ops[0]
            img1 = draw_tree(left_assoc_tree(op, operands), str(uuid.uuid4()))
            img2 = draw_tree(right_assoc_tree(op, operands), str(uuid.uuid4()))

        # PRECEDENCE
        elif "Precedence" in output and len(ops) >= 2:
            img1 = draw_tree(build_prec_tree(ops, operands), str(uuid.uuid4()))

            t2 = (operands[0],)
            for i in range(len(ops)):
                t2 = (ops[i], t2, (operands[i + 1],))

            img2 = draw_tree(t2, str(uuid.uuid4()))

        # DANGLING
        elif "Dangling" in output:
            t1, t2 = dangling_trees()
            img1 = draw_tree(t1, str(uuid.uuid4()))
            img2 = draw_tree(t2, str(uuid.uuid4()))

        # NONE
        else:
            t = (operands[0],)
            for i in range(len(ops)):
                t = (ops[i], t, (operands[i + 1],))

            img1 = draw_tree(t, str(uuid.uuid4()))

        return jsonify({
            "text": output,
            "img1": img1,
            "img2": img2
        })

    return jsonify({"text": "Invalid mode"})


# ---------------- MAIN ----------------
if __name__ == "__main__":
    os.makedirs("static", exist_ok=True)

    try:
        os.system('explorer.exe http://127.0.0.1:5000')
    except:
        pass

    app.run(debug=True, use_reloader=False)
