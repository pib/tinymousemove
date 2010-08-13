import socket
import struct
import sys
import time

CONNECT = struct.pack('<BxHHHHxx', 0x6c, 11, 0, 0, 0)


def parse_info(info_bin):
    vendor_len = struct.unpack('<H', info_bin[24:26])[0]
    formats_count = ord(info_bin[29])

    vendor_end = 40 + vendor_len
    vendor = info_bin[40:vendor_end]
    pad_len = vendor_len % 4
    screens_start = (formats_count * 8) + vendor_end + pad_len

    root = struct.unpack('<L', info_bin[screens_start:screens_start + 4])[0]
    return {
        'vendor': vendor,
        'root': root,
        }


def connect(socket_file):
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.connect(socket_file)
    s.send(CONNECT)
    res = s.recv(8)
    if ord(res[0]) == 1:
        res_len = struct.unpack('<H', res[-2:])[0] * 4
        rest = s.recv(res_len)
    else:
        raise IOError('error connecting to X server')
    info = parse_info(res + rest)
    return s, info


def warp(root, x, y):
    return struct.pack('<BxHLLHHHHhh', 41, 6, 0, root, 0, 0, 0, 0, x, y)


if __name__ == '__main__':
    if len(sys.argv) > 1:
        x = int(sys.argv[1])
        y = int(sys.argv[2])
    sock, info = connect('/tmp/.X11-unix/X0')
    sock.send(warp(info['root'], x, y))
    print 'Server vendor: %s' % info['vendor']
    sock.close()
