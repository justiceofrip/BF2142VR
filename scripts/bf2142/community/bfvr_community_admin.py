# Original admin module stays on the server; none of its source is distributed here.
import default
import bfvr_community

def init():
    default.init()
    bfvr_community.init()

def update():
    default.update()

def shutdown():
    default.shutdown()
