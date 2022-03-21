import xml.etree.ElementTree as ET

tree = ET.parse("models/worm.dae")
root = tree.getroot()

def named(i, what):
    return i.tag[i.tag.index('}') + 1:] == what

def get(of, what):
    for i in of:
        if named(i, what):
            return i
    print(of.tag, "doesn't have", what)

def walk(of, path):
    for i in path:
        of = get(of, i)
    return of

def print_children(of):
    for i in of:
        print(i.tag)


class Vertex:
    def __init__(self, position, normal, texture):
        self.position = position
        self.normal = normal
        self.texture = texture

    def __repr__(self):
        return '\n[' + str(self.position) + ', ' + str(self.normal) + ', ' + str(self.texture) + ']'

class MeshData:
    def __init__(self):
        self.vertices = []
        self.indices = []

geometries = get(root, "library_geometries")
for geometry in geometries:
    mesh_data = MeshData()

    print("Geometry:", geometry.get("id"), geometry.get("name"))
    positions = []
    normals = []
    textures = []

    mesh = get(geometry, "mesh")
    for source in mesh:
        if source.get("id") is None:
            continue

        if source.get("id").endswith("positions"):
            positions = [ float(i) for i in get(source, "float_array").text.split() ]
        elif source.get("id").endswith("normals"):
            normals = [ float(i) for i in get(source, "float_array").text.split() ]
        elif source.get("id").endswith("map-0"):
            textures = [ float(i) for i in get(source, "float_array").text.split() ]

    triangles = get(mesh, "triangles")
    indices = [ int(i) for i in get(triangles, "p").text.split() ]

    loaded = {}

    for i in range(0, len(indices), 3):
        p, n, t = indices[i], indices[i + 1], indices[i + 2]
        if (p, n, t) in loaded:
            print("loooooooooooooooooooooooooooooooooooooooooooooooooooooooooool")
            mesh_data.indices.append(loaded[(p, n, t)])
        else:
            index = len(loaded)
            mesh_data.vertices.append(Vertex(tuple(positions[p * 3:p * 3 + 3]),
                                        tuple(normals[n * 3:n * 3 + 3]),
                                        tuple(textures[t * 2: t * 2 + 2])))
            mesh_data.indices.append(index)
            loaded[(p, n, t)] = index
    print("Vertex data:")
    print(mesh_data.vertices)
    print("Index data:")
    print(mesh_data.indices)

controllers = get(root, "library_controllers")
for controller in controllers:
    skin = get(controller, "skin")

    print("Bind shape matrix:")
    print([ float(i) for i in get(skin, "bind_shape_matrix").text.split() ])

    names = []
    inverse_bind_matrices = []
    weight_data = []

    for source in skin:
        if not named(source, "source"):
            continue

        if source.get("id").endswith("skin-joints"):
            names += get(source, "Name_array").text.split()
        elif source.get("id").endswith("skin-bind_poses"):
            values = [ float(i) for i in get(source, "float_array").text.split() ]
            inverse_bind_matrices += [ values[i:i+16] for i in range(0, len(values), 16) ]
        elif source.get("id").endswith("skin-weights"):
            weight_data += [ float(i) for i in get(source, "float_array").text.split() ]

    vertex_weights = get(skin, "vertex_weights")

    weight_vcounts = [ int(i) for i in get(vertex_weights, "vcount").text.split() ]
    weight_verts = [ int(i) for i in get(vertex_weights, "v").text.split() ]
    print("sum", sum(weight_vcounts) * 2)
    print("len", len(weight_verts))

    weights = []
    index = 0
    for count in weight_vcounts:
        vertex_inds = []
        vertex_weights = []

        for i in range(index, index + count * 2, 2):
            vertex_inds.append(weight_verts[i])
            vertex_weights.append(weight_data[weight_verts[i + 1]])

        weights.append([ vertex_inds, vertex_weights ])

        index += count * 2


    print("Names:")
    print(names)
    print("Inverse bind matrices:")
    print(inverse_bind_matrices)
    print("Weights (len):", len(weights))
    print(weights)

