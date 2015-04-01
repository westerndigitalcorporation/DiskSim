
class Source:
    def __init__(self, s):
        self.fn = s

    def marshal(self):
        return "source %s" % (self.fn)

class Block:
    def __init__(self, typename, d, name = ""):
        self.name = name
        self.typestr = typename
        self.names = d

    def marshal(self):
        r = ""
        r += " %s %s " % (self.typestr, self.name)
        r += "{\n"
        keys =  self.names.keys()
        ii = 0
        for k in keys:
            r += "%s = %s" % (k, marshal(self.names[k]))
            if ii < (len(keys) - 1):
                r += ',\n'
            ii += 1
        
        r += "\n}"

        return r

    

def __init__(self, f):
    self.f = f

class List:
    def __init__(self,l,  n = 0):
        self.n = n   # how often to print a newline -- pp hint
        self.l = l
    
    def marshal(self):
        r = ""
        r += "[\n"
        ii = 0
        
        for i in self.l:
            r += marshal(i)
            if ii < (len(self.l) - 1):
                r += ', '
            if self.n and (not self.n \
                           or (ii and (ii % self.n == self.n - 1))):
                r += '\n'
            ii += 1
        
        r += " ]"

        return r


def marshal(x):
    r = ""
    if type(x) == int:
        r += ("%d" % x)
    elif type(x) == float:
        if x == 0.0:
            r += "0.0"
        else:
            r += ("%f" % x)
    elif type(x) == str:
        r += x
    else:
        r += x.marshal()

    return r



