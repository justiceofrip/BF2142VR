# BF2142 server native-connection proof bridge. Python 2.3 compatible.
# Install beside the server's existing python modules; init() after admin init.
import os, socket, struct, time
import host, bf2
_sock = None
_token = None
_port = 17572
_epoch = 0
_generation = 0
_players = {}
_timer = None
_last = 0
_COMMAND = 0x42564631

def _connected(player):
    global _generation
    if player.isAIPlayer(): return
    _generation += 1
    _players[player.index] = (_generation, player)

def _disconnected(player):
    if player.index in _players: del _players[player.index]

def _send(kind, payload):
    if _sock is None: return
    try: _sock.sendto('BFP1' + _token + struct.pack('<BI', kind, _epoch) + payload, ('127.0.0.1', _port))
    except socket.error: pass

def _proof(command, issuer, args):
    if command != _COMMAND or issuer is None or issuer.isAIPlayer() or len(args) != 4: return
    entry = _players.get(issuer.index)
    if entry is None or entry[1] is not issuer: return
    words = []
    for arg in args: words.append(long(arg) & 0xffffffffL)
    _send(1, struct.pack('<BI4I', issuer.index, entry[0], words[0], words[1], words[2], words[3]))

def _tick(data=None):
    # Full roster heartbeat removes departed/replaced slots even if no voice/poses arrive.
    current = {}
    for p in bf2.playerManager.getPlayers():
        if p.isAIPlayer(): continue
        old = _players.get(p.index)
        if old is None or old[1] is not p: _connected(p)
        current[p.index] = _players[p.index]
    _players.clear()
    _players.update(current)
    body = struct.pack('<H', len(current))
    for idx in current: body += struct.pack('<BI', idx, current[idx][0])
    _send(2, body)

def init():
    global _sock, _token, _port, _epoch, _timer
    if _sock is not None: return
    try:
        # BF2142's restricted os module has no environ/getenv.
        path = host.sgl_getOverlayDirectory() + 'admin/bfvr_community.key'
        fields = open(path, 'rb').read(256).strip().split(':')
        if len(fields) != 3 or len(fields[0]) != 64: raise ValueError('invalid proof config')
        _token = ''.join([chr(int(fields[0][i:i+2],16)) for i in range(0,64,2)])
        _port = int(fields[1]); _epoch = long(fields[2])
        if _port < 1 or _port > 65535 or _epoch < 1 or _epoch > 0xffffffffL: raise ValueError('invalid proof config')
        _sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        _sock.setblocking(0)
        host.registerHandler('PlayerConnect', _connected, 1)
        host.registerHandler('PlayerDisconnect', _disconnected, 1)
        host.registerHandler('ClientCommand', _proof, 1)
        _timer = bf2.Timer(_tick, 1, 1)
        _timer.setRecurring(1)
        _tick()
        print 'BF2142 community native player proof ready (loopback only).'
    except Exception, e:
        _sock = None
        print 'BF2142 community native proof unavailable; network addon authorization stays closed.'
