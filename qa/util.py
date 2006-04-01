import random

def str_random_generate (n):
    c = ""
    x = int (random.random() * 100)
    y = int (random.random() * 100)
    z = int (random.random() * 100)

    for i in xrange(n):
        x = (171 * x) % 30269
        y = (172 * y) % 30307
        z = (170 * z) % 30323
        c += chr(32 + int((x/30269.0 + y/30307.0 + z/30323.0) * 1000 % 95))

    return c

def letters_random_generate (n):
    c = ""
    x = int (random.random() * 100)
    y = int (random.random() * 100)
    z = int (random.random() * 100)

    for i in xrange(n):
        x = (171 * x) % 30269
        y = (172 * y) % 30307
        z = (170 * z) % 30323
        if z%2:
            c += chr(65 + int((x/30269.0 + y/30307.0 + z/30323.0) * 1000 % 25))
        else:
            c += chr(97 + int((x/30269.0 + y/30307.0 + z/30323.0) * 1000 % 25))
            
    return c


str_buf     = ""
letters_buf = ""

def letters_random (n):
    global letters_buf

    letters_len = len(letters_buf)
    if letters_len == 0:
        letters_random_generate (1000)
        letters_len = len(letters_buf)

    offset = random.randint (0, letters_len)

    if letters_len - offset > n:
        return letters_buf[offset:offset+n]

    tmp = letters_random_generate (n - (letters_len - offset))
    letters_buf += tmp

    return letters_buf[offset:]


def str_random (n):
    global str_buf

    str_len = len(str_buf)
    if str_len == 0:
        str_random_generate (1000)
        str_len = len(str_buf)

    offset = random.randint (0, str_len)

    if str_len - offset > n:
        return str_buf[offset:offset+n]

    tmp = str_random_generate (n - (str_len - offset))
    str_buf += tmp
    return str_buf[offset:]

