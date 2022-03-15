import xml.etree.ElementTree as ET

tree = ET.parse("models/worm.dae")
root = tree.getroot()

def get(of, what):
    for i in of:
        if i.tag.endswith(what):
            return i
    print(of.tag, "doesn't have", what)

def walk(of, path):
    for i in path:
        of = get(of, i)
    return of

def print_children(of):
    for i in of:
        print(i.tag)

cameras = get(root, "library_cameras")
print_children(cameras)
animation = walk(root, ["library_animations", "animation"])
for a in animation:
    print("")
    print_children(a)
