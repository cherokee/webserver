def validate_boolean (value):
    if value.lower() in ['on', '1', 'true']:
        return '1'
    return '0'

def validate_tcp_port (value):
    try:
        tmp = int(value)
    except:
        raise ValueError, 'Port must be a number'
    if tmp < 0 or tmp > 0xFFFF - 1:
        raise ValueError, 'Out of the range (1 to 65534)'
    return value

def validate_path (value):
    if not value:
        raise ValueError, 'Path cannot be empty'
    if value[0] == '/':
        return value
    if value[1:3] == ":\\":
        return value
    raise ValueError, 'Malformed path'

def validate_path_list (value):
    re = []
    for p in value.split(','):
        re.append(validate_path(p))
    return reduce(lambda x,y: x+','+y, re)

def validate_positive_int (value):
    tmp = int(value)
    if tmp < 0:
        raise ValueError, 'It cannot be negative'
    return value
