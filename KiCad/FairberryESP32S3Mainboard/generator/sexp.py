import re

def tokenize(s):
    tokens = []
    i = 0
    n = len(s)
    while i < n:
        c = s[i]
        if c.isspace():
            i += 1
        elif c == '(' or c == ')':
            tokens.append(c)
            i += 1
        elif c == '"':
            j = i + 1
            buf = []
            while j < n and s[j] != '"':
                if s[j] == '\\' and j+1 < n:
                    buf.append(s[j+1])
                    j += 2
                else:
                    buf.append(s[j])
                    j += 1
            tokens.append(('str', ''.join(buf)))
            i = j + 1
        else:
            j = i
            while j < n and not s[j].isspace() and s[j] not in '()':
                j += 1
            tokens.append(('atom', s[i:j]))
            i = j
    return tokens

def parse(tokens):
    pos = [0]
    def parse_expr():
        tok = tokens[pos[0]]
        if tok == '(':
            pos[0] += 1
            items = []
            while tokens[pos[0]] != ')':
                items.append(parse_expr())
            pos[0] += 1
            return items
        elif isinstance(tok, tuple):
            pos[0] += 1
            return tok[1]
        else:
            raise ValueError(f"unexpected {tok}")
    exprs = []
    while pos[0] < len(tokens):
        exprs.append(parse_expr())
    return exprs

def parse_file(path):
    with open(path) as f:
        content = f.read()
    return parse(tokenize(content))[0]  # top-level single expr (kicad_symbol_lib ...)

def find_all(tree, head):
    """Find all sub-lists whose first element equals head, recursively."""
    results = []
    if isinstance(tree, list):
        if tree and tree[0] == head:
            results.append(tree)
        for item in tree:
            results.extend(find_all(item, head))
    return results

def find_direct(tree, head):
    """Find direct children lists whose first element equals head (no recursion into matches)."""
    results = []
    if isinstance(tree, list):
        for item in tree:
            if isinstance(item, list) and item and item[0] == head:
                results.append(item)
    return results
