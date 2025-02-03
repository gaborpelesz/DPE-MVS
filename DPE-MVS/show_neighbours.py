import numpy as np
import cv2
import struct

def read_bin_mat(mat_path):
    with open(mat_path, 'rb') as f:
        version = np.frombuffer(f.read(4), dtype=np.int32)[0]
        if version != 1:
            print(f"Version error: {mat_path}")
            return False, None

        rows = np.frombuffer(f.read(4), dtype=np.int32)[0]
        cols = np.frombuffer(f.read(4), dtype=np.int32)[0]
        mat_type = np.frombuffer(f.read(4), dtype=np.int32)[0]

        if mat_type == cv2.CV_8U:
            dtype = np.uint8
        elif mat_type == cv2.CV_32F:
            dtype = np.float32
        elif mat_type == cv2.CV_32S:
            dtype = np.int32
        else:
            print(f"Unsupported matrix type: {mat_type}")
            return None

        mat = np.empty((rows, cols), dtype=dtype)
        f.readinto(mat.data)

        return mat

def read_neighbour_bin(file_path, map_index):
    with open(file_path, 'rb') as f:
        # 读取第一个整数（weak_count）
        weak_count_data = f.read(4)
        weak_count = struct.unpack('i', weak_count_data)[0]
        
        # 读取第二个整数（NEIGHBOUR_NUM）
        neighbour_sample_num_data = f.read(4)
        neighbour_sample_num = struct.unpack('i', neighbour_sample_num_data)[0]
        
        offset = neighbour_sample_num * map_index * 4
        print('offset:', offset)

        f.seek(offset, 1)  # offset个字节相对于当前位置
        
        # 读取从偏移位置开始的short2结构体
        neighbours = []
        for _ in range(neighbour_sample_num):
            short2_data = f.read(4)  # 每个 short2 结构体是 4 个字节
            x, y = struct.unpack('hh', short2_data)
            neighbours.append((x, y))

        # # 读取所有的 short2 结构体
        # neighbours = []
        # for _ in range(weak_count * neighbour_sample_num):
        #     short2_data = f.read(4)  # 每个 short2 结构体是 4 个字节
        #     x, y = struct.unpack('hh', short2_data)
        #     neighbours.append((x, y))
    
    return weak_count, neighbour_sample_num, neighbours


neighbour_map_path = "/mnt/sharedisk/chenkehua/ETH3D/training/pipes/SAPD/00000013/neighbour_map.bin"
neighbour_path = "/mnt/sharedisk/chenkehua/ETH3D/training/pipes/SAPD/00000013/neighbour.bin"
# image_path = "/mnt/sharedisk/chenkehua/ETH3D/training/pipes/images/00000013.jpg"
image_path = "/mnt/sharedisk/chenkehua/ETH3D/training/pipes/SAPD/00000000/normal_14.jpg"

neighbour_map = read_bin_mat(neighbour_map_path)
print("(rows, cols): ", neighbour_map.shape)
# print("neighbour_map:\n", neighbour_map)

debug_x = 3022
debug_y = 330
scale_size = 3

map_index = neighbour_map[debug_y][debug_x]
print('map_index:', map_index)
weak_count, neighbour_sample_num, neighbours = read_neighbour_bin(neighbour_path, map_index)
print("weak_count:", weak_count)
print("neighbour_sample_num:", neighbour_sample_num)
print("neighbours:", neighbours)

# print(neighbour_map[debug_y][debug_x])

# for i in range(neighbour_sample_num):
#     print(neighbours[offset + i])
img = cv2.imread(image_path)

# factor = 1.0 / scale_size;
# new_cols = round(img.shape[1] * factor)
# new_rows = round(img.shape[0] * factor)

# img = cv2.resize(img, (new_cols, new_rows), interpolation=cv2.INTER_LINEAR)

for i, point in enumerate(neighbours):
    if i == 0:
        if point[0] != debug_x or point[1] != debug_y:
            print("NOT WEAK POINT")
            break
        cv2.circle(img, point, 5, (0, 0, 255), -1)
    else:
        cv2.circle(img, point, 5, (0, 255, 0), -1)

cv2.imwrite('./neighbour.jpg', img)