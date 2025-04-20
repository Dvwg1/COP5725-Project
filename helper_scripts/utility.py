import struct
import sys

PAGE_SIZE = 8000
RECORD_SIZE = 66

def parse_leaf(data):
    is_leaf, num_records, next_leaf = struct.unpack_from('<iii', data, 0)
    print("== LEAF NODE ==")
    print("Num records:", num_records)
    print("Next leaf:", next_leaf)

    offset = 12
    for i in range(num_records):
        record = struct.unpack_from('<25sf f29si', data, offset)
        id_bytes, lon, lat, ts_bytes, hilbert = record
        id_str = id_bytes.decode('utf-8', errors='ignore').rstrip('\x00')
        ts_str = ts_bytes.decode('utf-8', errors='ignore').rstrip('\x00')
        print(f"[{i}] ID: {id_str}, Lat: {lat:.6f}, Lon: {lon:.6f}, Time: {ts_str}, Hilbert: {hilbert}")
        offset += RECORD_SIZE

def parse_internal(data):
    is_leaf, num_keys = struct.unpack_from('<ii', data, 0)
    print("== INTERNAL NODE ==")
    print("Num keys:", num_keys)

    if num_keys < 0 or num_keys > 255:
        print("Invalid num_keys value:", num_keys)
        return

    keys_offset = 8
    keys = struct.unpack_from(f'<{num_keys}i', data, keys_offset)

    children_offset = keys_offset + num_keys * 4
    children = struct.unpack_from(f'<{num_keys + 1}i', data, children_offset)

    print("Keys:", list(keys))
    print("Children:", list(children))

    print("\n[DEBUG] Raw bytes (first 64):", list(data[:64]))
    print(f"[DEBUG] Children bytes ({children_offset}-{children_offset + (num_keys+1)*4}):",
          list(data[children_offset:children_offset + (num_keys + 1) * 4]))



def main():
    if len(sys.argv) < 2:
        print("Usage: python dump_page.py <page_file>")
        return

    with open(sys.argv[1], 'rb') as f:
        data = f.read(PAGE_SIZE)

    is_leaf = struct.unpack_from('<i', data, 0)[0]
    if is_leaf:
        parse_leaf(data)
    else:
        parse_internal(data)

if __name__ == '__main__':
    main()

