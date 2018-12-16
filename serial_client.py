#!/usr/bin/env python3
import asyncio
import serial_asyncio
from colorama import init, Fore, Back, Style

init(autoreset=True)


async def main(loop):
    reader, writer = await serial_asyncio.open_serial_connection(
        url='/dev/cu.wchusbserial1410', baudrate=115200
    )
    received = recv(reader, "CAN Init")
    await asyncio.wait([received])
    sent = send(writer, [b'D'])
    receiver = recv(reader)

    await asyncio.wait([sent, received, receiver])


async def send(w, msgs):
    for msg in msgs:
        w.write(msg)
        print(f'sent: {msg.decode().rstrip()}')


async def recv(r, msgr=None):
    while True:
        msg = await r.readuntil(b'\n')
        print(f'received: {mfmt(msg.rstrip().decode())}')
        if (msgr is not None) and msgr.strip() in msg.strip().decode():
            break


# format messages
def mfmt(msg):
    msgtype = Fore.RESET
    m = msg.split("] ", 1)
    if len(m) > 1:
        if m[0] == 'I':
            msgtype = Fore.LIGHTBLUE_EX
            return msgtype + m[1]
        elif m[0] == 'D':
            msgtype = Fore.LIGHTYELLOW_EX
            return msgtype + dfmt(m[1])
    else:
        return m


# decode channel setup timing parameters
def decode_chan_tparam(byt):
    bunit = byt >> 6
    scale = byt & 0b0011_1111
    unit = 0

    if bunit == 0x0:
        unit = 0.1  # ms
    elif bunit == 0x1:
        unit = 1  # ms
    elif bunit == 0x2:
        unit = 10  # ms
    elif bunit == 0x3:
        unit = 100  # ms

    return '{}ms'.format(unit*scale)


# format data messages


def dfmt(msg):
    # See: http://jazdw.net/tp20

    m = msg.split("  ")
    if not len(m) > 1:
        return msg

    # message
    mid = m[0]  # message id
    msgh = m[1]  # message hex data

    # decode msgh hex str as bytes
    mbytes = bytes.fromhex(msgh)
    op = "?UNKN"

    # split first byte for data packet
    dataop = (mbytes[0] & 0b1111_0000) >> 4
    seq = (mbytes[0] & 0b0000_1111)

    # packet type
    ptype = 0  # Bcast=1, ChanSetup=2, ChanParam=3, Data=4

    # packet dict
    packet = dict()

    # message data
    mdata = ""

    # identify different packet types and decode op codes for them
    if m[0] == "0x740" or m[0] == "0x300" and \
        (len(mbytes) >= 1 and len(mbytes) <= 8) and \
        ((dataop >= 0b0000 and dataop <= 0b0011)
         or dataop == 0b1011 or dataop == 0b1001):
            # decode opcode data transmission
        ptype = 4

        if dataop == 0x0:
            op = "DATA_WAIT_ACK_MOREPACKETS({})".format(hex(seq))
        elif dataop == 0x1:
            op = "DATA_WAIT_ACK_LASTPACKET({})".format(hex(seq))
        elif dataop == 0x2:
            op = "DATA_NOWAIT_ACK_MOREPACKETS({})".format(hex(seq))
        elif dataop == 0x3:
            op = "DATA_NOWAIT_ACK_LASTPACKET({})".format(hex(seq))
        elif dataop == 0xB:
            op = "DATA_READY_NEXT_PACKET({})".format(hex(seq))
        elif dataop == 0x9:
            op = "DATA_NOTREADY_NEXT_PACKET({})".format(hex(seq))

    if (len(mbytes) == 1 or len(mbytes) == 6) and \
            ((mbytes[0] >> 4) == 0b1010):
        # decode opcode for CHAN_PARAMS
        ptype = 3

        if mbytes[0] == 0xA0:
            op = "CHAN_PARAMS_REQ"
        elif mbytes[0] == 0xA1:
            op = "CHAN_PARAMS_RESP"
        elif mbytes[0] == 0xA3:
            op = "CHAN_TEST"
        elif mbytes[0] == 0xA4:
            op = "CHAN_BREAK"
        elif mbytes[0] == 0xA8:
            op = "CHAN_DISCONNECT"

        if len(mbytes) == 6:
            # block size
            packet['bs'] = mbytes[1]

            # timing
            packet['t1'] = decode_chan_tparam(mbytes[2])
            # packet['t2'] = decode_chan_tparam(mbytes[3])  # always 0b111111
            packet['t3'] = decode_chan_tparam(mbytes[4])
            # packet['t4'] = decode_chan_tparam(mbytes[5])  # always 0b111111

            if packet is not None:
                mdata = "BS={} T1={} T3={}".format(
                    packet['bs'], packet['t1'], packet['t3'])

    if len(mbytes) == 7:
        # decode opcode for BCAST

        if mbytes[1] == 0x23:
            ptype = 1
            op = "BCAST_REQ"
        elif mbytes[1] == 0x24:
            ptype = 1
            op = "BCAST_RESP"

        if mbytes[5] == 0x00:
            ptype = 1
            respreq = "KWP_RESP_EXPECTED"
        elif mbytes[5] == 0x55 or mbytes[5] == 0xAA:
            ptype = 1
            respreq = "KWP_RESP_NOTEXPECTED"

        # decode opcode for Channel Setup
        if mbytes[1] == 0xC0:
            ptype = 2
            op = "CHAN_SETUP_REQ"
        elif mbytes[1] == 0xD0:
            ptype = 2
            op = "CHAN_SETUP_POS_RESP"
        elif mbytes[1] >= 0xD6 and mbytes[1] <= 0xD8:
            ptype = 2
            op = "CHAN_SETUP_NEG_RESP"

        # FIXME
        # packet['rxid'] = mbytes[2]

        # packet['rxv'] = (mbytes[3] & 0b1111_1100) >> 2
        # packet['rxpref'] = (mbytes[3] & 0b0000_0011) << 8

        # packet['txid'] = mbytes[4]

        # packet['txv'] = (mbytes[5] & 0b1111_1100) >> 2
        # packet['txpref'] = (mbytes[5] & 0b0000_0011) << 8

        # packet['app'] = mbytes[6]

        # print('RX: {}  RXV:{}  TX: {}  TXV:{}  app:{}'.format(
        #     hex(packet['rxpref'] & packet['rxid']),
        #     hex(packet['rxv']),
        #     hex(packet['txpref'] & packet['txid']),
        #     hex(packet['txv']),
        #     hex(packet['app'])
        # ))

    # format data packets
    # ptype: Bcast=1, ChanSetup=2, ChanParam=3, Data=4
    if ptype == 4 and len(mbytes) >= 2:
        if seq == 0x0 and ((mbytes[0] & 0x10) == 0x1):
            # FIXME
            data = mbytes[2:]
            datalen = mbytes[1]
            mdata = hex(datalen) + " " + data.hex()
        else:
            data = mbytes[1:]
            mdata = data.hex()

    if ptype == 3 and len(mbytes) == 2:
        mdata += "  seq:".format(hex(seq))

    # format packet type
    pmsg = "?UNKN"
    if ptype == 1:
        pmsg = "BCAST"
    elif ptype == 2:
        pmsg = "SETUP"
    elif ptype == 3:
        pmsg = "PARAM"
    elif ptype == 4:
        pmsg = "DATA"

    # format message id
    if m[0] == "0x740":
        mid = "0x740 CLIENT "
    elif m[0] == "0x300":
        mid = "0x300 ECU "

    # prepare return string
    reset = Back.RESET+Fore.WHITE
    return '{}{:13.13}{}'.format(Fore.RED, mid, reset) + \
           '|{}{:2.2}{}'.format(Fore.CYAN, str(len(mbytes)), reset) + \
           '|{}{:5.5}{}'.format(Fore.YELLOW, pmsg, reset) + \
           '|{}{:34.34}{}'.format(Fore.MAGENTA, op, reset) + \
           '|{}{:36}{}'.format(Fore.BLUE, mdata.strip(), reset) + \
           '|{}{}{}'.format(Fore.GREEN, mbytes.hex(), reset)


# begin async
loop = asyncio.get_event_loop()
loop.run_until_complete(main(loop))
loop.close()
