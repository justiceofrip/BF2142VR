"""Bounded repair decisions only: no mesh coordinates, indices or textures."""
import base64, zlib
MAX_DECISIONS=2_000_000

def encode(decisions):
    raw=bytes(decisions)
    if len(raw)>MAX_DECISIONS or any(x>2 for x in raw):raise ValueError('Invalid repair decisions')
    return base64.b85encode(zlib.compress(raw,9)).decode('ascii')

class DecisionPlan:
    def __init__(self,encoded):
        if len(encoded)>MAX_DECISIONS:raise ValueError('Oversized repair plan')
        decoder=zlib.decompressobj()
        self.data=decoder.decompress(base64.b85decode(encoded),MAX_DECISIONS+1)
        if len(self.data)>MAX_DECISIONS or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail:
            raise ValueError('Invalid repair plan stream')
        self.at=0
    def take(self,maximum):
        if self.at>=len(self.data):raise ValueError('Truncated repair plan')
        value=self.data[self.at];self.at+=1
        if value>maximum:raise ValueError('Invalid repair decision')
        return value
    def finish(self):
        if self.at!=len(self.data):raise ValueError('Unused repair decisions')
