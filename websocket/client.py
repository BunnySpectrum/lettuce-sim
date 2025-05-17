#!/usr/bin/env python

"""Client using the asyncio API."""

import asyncio
from websockets.asyncio.client import connect, ClientConnection
import websockets
from typing import Optional

inbox = []

def process_data(data: str):
    msg = []
    for i in range(len(data)//2):
        msg.append(int(data[2*i: 2*i+2],16))

    opcode = msg[0]
    if opcode in [1, 2]:
        result = ''.join([chr(_) for _ in msg[1:]]).rstrip()
        result = result.replace('\x00','').strip()
        if len(result) > 0:
            print(f'{result}')
    elif opcode in [0]:
        pass
        # print(msg[1:])
        # print(f'Control: op={msg[1]}, size={msg[2]}, data={msg[3:]}')
    else:
        print(f'Got: {data}')

async def read(ws: ClientConnection):
    while True:
        try:
            data = await ws.recv(decode=False)
            inbox.append(data)
            process_data(data.hex())
        except websockets.ConnectionClosedError:
            print("Read: closed error")
            return

outbox = asyncio.Queue()

async def sender(ws: ClientConnection):
    while True:
        try:
            try:
                msg = await outbox.get()
            except asyncio.QueueShutDown:
                print("Queue closed")
                return

            print(f'Send: {msg}')
            await ws.send(message=msg)
            print(f'\tdone')
        except websockets.ConnectionClosedError:
            print("Sender: Closed error")
            return

def to_uint16(value, msb_first=True) -> bytes:
    if msb_first:
        order = [8, 0]
    else:
        order = [0, 8]

    return bytes([(value>>_)&0xff for _ in order])

import code
import os
class Emu:
    def __init__(self, loop: asyncio.AbstractEventLoop):
        self._loop = loop

    def send(self, msg: bytes):
        asyncio.run_coroutine_threadsafe(outbox.put(msg), self._loop).result()

    def cmd(self, msg: str):
        self.send(b'\x04' + bytes(msg, 'utf-8'))

    def dump(self, addr):
        self.cmd(f'dump {addr}\n')

    def help(self):
        self.cmd('help\n')

    def upload_elf(self, path: str, name: Optional[str] = None):
        if name is None:
            name = os.path.basename(path)
        with open(path, 'rb') as elf:
            data = elf.read()   
            fileSize = len(data)
            fileNameLength = len(name)
            print(f'Name: {name}, Size: {fileSize}')
            self.send(b'\x00' + to_uint16(fileSize) + to_uint16(fileNameLength) + bytes(name, 'utf-8'))
            self.send(data)



def shutdown(ws: ClientConnection, queue: asyncio.Queue):
    queue.shutdown()
    # ws.close()

#  run			[Run Program Until Breakpoint is Hit]
#  step [N]		[Step Into Instruction]
#  dump [HEX_ADDR|Rn]	[Dump Memory direct or at register value]
#  set [HEX_ADDR|Rn]	[Set Memory or Register Location]
#  dis [N][HEX_ADDR]	[Disassemble Instructions]
#  break ADDR		[Set a Breakpoint]
#  bps			[Display Breakpoints]
#  regs			[Display Registers]
#  reset			[Reset Machine]
#  quit			[Exit program]

from IPython import embed
def repl(loop: asyncio.AbstractEventLoop, ws: ClientConnection):
    print('Start repl')
    reason = 0
    locals = globals()
    locals['loop'] = loop
    locals['send'] = lambda msg: asyncio.run_coroutine_threadsafe(outbox.put(msg), loop).result()
    locals['dump'] = lambda addr: b'\x04' + bytes(f'dump {addr}\n', 'utf-8')
    emu = Emu(loop)
    try:
        embed()
        # code.interact(local=locals)
    except SystemExit as e:
        # outbox.shutdown()
        # print(f'{e=}, {e.code}, {type(e.code)}')
        # return e.code == 1
        print("Shutting down")
        shutdown(ws, outbox)
    return False


import time
async def transact():
    loop = asyncio.get_running_loop()
    keep_running = True
    while keep_running:
        print("Connecting...")
        try:
            conn = await connect("ws://localhost:9000", subprotocols=['emu-protocol'], open_timeout=5)
        except OSError:
            time.sleep(1)
            continue

        print("\tConnected!")
        websocket = conn
        # async with connect("ws://localhost:9000", subprotocols=['emu-protocol'], open_timeout=5) as websocket:
            # async with asyncio.TaskGroup() as tg:
            #     task1 = tg.create_task(read(websocket))
            #     task2 = tg.create_task(sender(websocket))
            #     task3 = tg.create_task(asyncio.to_thread(repl, loop, websocket))
        task1 = asyncio.create_task(read(websocket))
        task2 = asyncio.create_task(sender(websocket))
        task3 = asyncio.create_task(asyncio.to_thread(repl, loop, websocket))
        done, pending = await asyncio.wait([task1, task2, task3], return_when=asyncio.FIRST_COMPLETED)
        for task in pending:
            task.cancel()
        await websocket.close()
        keep_running = True
        print("Bottom")



if __name__ == "__main__":
    print("Starting")
    asyncio.run(transact())
    print("Ending")
