# file format:

"""header"""
# vertex count
# index count
# bone count

"""data"""
# vertices              [(3f), (3f), (2f)]
# indices               [(i)]

# bone ids, weights     [(4i), (4f)]
# inverse bind matrices [(4x4f)]

# per bone frame counts [(i)]
# input time stamps     [[(f)]]
# output transforms     [[(4f), (4f)[]

# child counts          [(i)]
# bone hierarchy (ids)  [[(u)]]

import xml.etree.ElementTree as ET
import json
import struct
import math

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
    def __init__(self, position, normal, texture, pos_id):
        self.pos_id = pos_id
        self.position = position
        self.normal = normal
        self.texture = texture

    def __repr__(self):
        return '\n[' + str(self.pos_id) + ", " + str(self.position) + ', ' + str(self.normal) + ', ' + str(self.texture) + ']'

class MeshData:
    def __init__(self):
        self.vertices = []
        self.indices = []

def get_mesh_data(root):
    geometries = get(root, "library_geometries")
    for geometry in geometries:
        mesh_data = MeshData()

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

        print("position length:", len(positions) // 3)

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
                mesh_data.vertices.append(Vertex(
                                            tuple(positions[p * 3:p * 3 + 3]),
                                            tuple(normals[n * 3:n * 3 + 3]),
                                            tuple(textures[t * 2: t * 2 + 2]),
                                            p))
                mesh_data.indices.append(index)
                loaded[(p, n, t)] = index

        print("Vertex data:")
        print(mesh_data.vertices)
        print("Index data:")
        print(mesh_data.indices)

        return mesh_data


def get_controller_data(root):
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

        weights = []
        index = 0
        for count in weight_vcounts:
            vertex_inds = []
            vertex_weights = []

            for i in range(index, index + count * 2, 2):
                vertex_inds.append(weight_verts[i])
                vertex_weights.append(weight_data[weight_verts[i + 1]])

            weights.append(( vertex_inds, vertex_weights ))

            index += count * 2

        print("Names:")
        print(names)
        print("Inverse bind matrices:")
        print(inverse_bind_matrices)
        print("Weights (len):", len(weights))
        print(weights)

        return names, inverse_bind_matrices, weights


def get_frame_data(root):
    bone_frames = []

    animation_bones = walk(root, ["library_animations", "animation"])
    for bone in animation_bones:
        frame_inputs = []
        frame_outputs = []

        for source in bone:
            if source.get("id") is None:
                continue

            if source.get("id").endswith("matrix-input"):
                frame_inputs += [ float(i) for i in get(source, "float_array").text.split() ]
            elif source.get("id").endswith("matrix-output"):
                values = [ float(i) for i in get(source, "float_array").text.split() ]
                frame_outputs += [ values[i:i + 16] for i in range(0, len(values), 16) ]

        bone_frames.append((frame_inputs, frame_outputs))
        
    print("Bone frames:")
    print(bone_frames)

    return bone_frames


def get_skeleton_data(root):
    bone_matrices = {}

    armature = walk(root, ["library_visual_scenes", "visual_scene", "node"])

    def walk_skeleton(parent):
        sid = parent.get("sid")
        bone_matrices[sid] = [ float(i) for i in get(parent, "matrix").text.split() ]
        children = {}

        for child in parent:
            if not named(child, "node"):
                continue
            children[child.get("sid")] = walk_skeleton(child)

        return children

    root_bone = get(armature, "node")
    hierarchy = { root_bone.get("sid"): walk_skeleton(root_bone) }

    print(json.dumps(hierarchy, indent = 2))

    return hierarchy


def at(x, y):
    return 4 * y + x

# public static Quaternion fromMatrix(Matrix4f matrix) {
#         float w, x, y, z;
#         float diagonal = matrix.m00 + matrix.m11 + matrix.m22;
#         if (diagonal > 0) {
#                 float w4 = (float) (Math.sqrt(diagonal + 1f) * 2f);
#                 w = w4 / 4f;
#                 x = (matrix.m21 - matrix.m12) / w4;
#                 y = (matrix.m02 - matrix.m20) / w4;
#                 z = (matrix.m10 - matrix.m01) / w4;
#         } else if ((matrix.m00 > matrix.m11) && (matrix.m00 > matrix.m22)) {
#                 float x4 = (float) (Math.sqrt(1f + matrix.m00 - matrix.m11 - matrix.m22) * 2f);
#                 w = (matrix.m21 - matrix.m12) / x4;
#                 x = x4 / 4f;
#                 y = (matrix.m01 + matrix.m10) / x4;
#                 z = (matrix.m02 + matrix.m20) / x4;
#         } else if (matrix.m11 > matrix.m22) {
#                 float y4 = (float) (Math.sqrt(1f + matrix.m11 - matrix.m00 - matrix.m22) * 2f);
#                 w = (matrix.m02 - matrix.m20) / y4;
#                 x = (matrix.m01 + matrix.m10) / y4;
#                 y = y4 / 4f;
#                 z = (matrix.m12 + matrix.m21) / y4;
#         } else {
#                 float z4 = (float) (Math.sqrt(1f + matrix.m22 - matrix.m00 - matrix.m11) * 2f);
#                 w = (matrix.m10 - matrix.m01) / z4;
#                 x = (matrix.m02 + matrix.m20) / z4;
#                 y = (matrix.m12 + matrix.m21) / z4;
#                 z = z4 / 4f;
#         }
#         return new Quaternion(x, y, z, w);
# }
def to_quaternion(mat):
    diagonal = mat[at(0, 0)] + mat[at(1, 1)] + mat[at(2, 2)]
    if diagonal > 0:
        w4 = math.sqrt(diagonal + 1.0) * 2.0
        w = w4 / 4.0
        x = (mat[at(2, 1)] - mat[at(1, 2)]) / w4
        y = (mat[at(0, 2)] - mat[at(2, 0)]) / w4
        z = (mat[at(1, 0)] - mat[at(0, 1)]) / w4
    elif mat[at(0, 0)] > mat[at(1, 1)] and mat[at(0, 0)] > mat[at(2, 2)]:
        x4 = math.sqrt(1.0 + mat[at(0, 0)] - mat[at(1, 1)] - mat[at(2, 2)]) * 2.0
        w = (mat[at(2, 1)] - mat[at(1, 2)]) / x4
        x = x4 / 4.0
        y = (mat[at(0, 1)] + mat[at(1, 0)]) / x4
        z = (mat[at(0, 2)] + mat[at(2, 0)]) / x4
    elif mat[at(1, 1)] > mat[at(2, 2)]:
        y4 = math.sqrt(1.0 + mat[at(1, 1)] - mat[at(0, 0)] - mat[at(2, 2)]) * 2.0
        w = (mat[at(0, 2)] - mat[at(2, 0)]) / y4
        x = (mat[at(0, 1)] + mat[at(1, 0)]) / y4
        y = y4 / 4.0
        z = (mat[at(1, 2)] + mat[at(2, 1)]) / y4
    else:
        z4 = math.sqrt(1.0 + mat[at(2, 2)] - mat[at(0, 0)] - mat[at(1, 1)]) * 2.0
        w = (mat[at(1, 0)] - mat[at(0, 1)]) / z4
        x = (mat[at(0, 2)] + mat[at(2, 0)]) / z4
        y = (mat[at(1, 2)] + mat[at(2, 1)]) / z4
        z = z4 / 4.0
    return w, x, y, z

def to_position(mat):
    return mat[at(3, 0)], mat[at(3, 1)], mat[at(3, 2)], 1.0


tree = ET.parse("models/worm.dae")
root = tree.getroot()

mesh_data = get_mesh_data(root)
bone_names, ibms, weights = get_controller_data(root)
bone_frames = get_frame_data(root)
skeleton = get_skeleton_data(root)


f = open("output.bin", "wb")

# header
f.write(struct.pack("i", len(mesh_data.vertices)))
f.write(struct.pack("i", len(mesh_data.indices)))
f.write(struct.pack("i", len(bone_names)))

# vertices
for vertex in mesh_data.vertices:
    f.write(struct.pack("=fff", *vertex.position))
    f.write(struct.pack("=fff", *vertex.normal))
    f.write(struct.pack("=ff", *vertex.texture))

#indices
f.write(struct.pack("=" + "I" * len(mesh_data.indices), *mesh_data.indices))

# bone ids, weights
for vertex in mesh_data.vertices:
    zipped = list(zip(*weights[vertex.pos_id]))
    zipped.sort(key = lambda x: x[1], reverse = True)

    if len(zipped) < 4:
        zipped += [(0, 0.0)] * (4 - len(zipped))
    elif len(zipped) > 4:
        zipped = zipped[:4]

    ids = [ i for i, _ in zipped ]
    weits = [ i for _, i in zipped ]

    length = math.sqrt(sum([i * i for i in weits]))
    weits = [ i / length for i in weits ]

    f.write(struct.pack("=IIII", *ids))
    f.write(struct.pack("=ffff", *weits))

#inverse bind matrices
for ibm in ibms:
    f.write(struct.pack("=" + "f" * 16, *ibm))

# per bone frame counts
for bone in bone_frames:
    f.write(struct.pack("i", len(bone[0])))

# input time stamps
for bone in bone_frames:
    f.write(struct.pack("=" + "f" * len(bone[0]), *bone[0]))

# output transforms
for bone in bone_frames:
    for transform in bone[1]:
        position = to_position(transform)
        rotation = to_quaternion(transform)
        f.write(struct.pack("=ffff", *position))
        f.write(struct.pack("=ffff", *rotation))

# child counts
child_counts = [-1] * len(bone_names)

def walk_bones_counts(children):
    global child_counts
    global bone_names

    for bone_name in children:
        child_counts[bone_names.index(bone_name)] = len(children[bone_name])
        walk_bones_counts(children[bone_name])

walk_bones_counts(skeleton)
f.write(struct.pack("=" + "I" * len(child_counts), *child_counts))

# bone hierarchy (ids)

def walk_bones(children):
    global f
    global bone_names

    ids = [ bone_names.index(name) for name in children ]
    f.write(struct.pack("=" + "I" * len(ids), *ids))
    for name in children:
        walk_bones(children[name])

walk_bones(next(iter(skeleton.values())))

f.close()

# hierarchy


# file format:

"""header"""
# vertex count
# index count
# bone count

"""data"""
# vertices              [(3f), (3f), (2f)]
# indices               [(i)]

# bone ids, weights     [(4i), (4f)]
# inverse bind matrices [(4x4f)]

# per bone frame counts [(i)]
# input time stamps     [[(f)]]
# output transforms     [[(4f), (4f)]]

# child counts          [(i)]
# bone hierarchy (ids)  [[(i)]]













