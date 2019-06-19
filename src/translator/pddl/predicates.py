class Predicate(object):
    def __init__(self, name, arguments):
        self.name = name
        self.arguments = arguments
        self.static = False

    def __str__(self):
        return "%s(%s)" % (self.name, ", ".join(map(str, self.arguments)))

    def get_arity(self):
        return len(self.arguments)

    def set_static(self):
        self.static = True
        return
