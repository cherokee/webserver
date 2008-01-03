def validate_boolean (value):
    if value.lower() in ['on', '1', 'true']:
        return '1'
    return '0'

def validate_tcp_port (value):
    try:
        tmp = int(value)
    except:
        raise "Invalid port"        
    if tmp < 0 or tmp > 0xFFFF - 1:
        raise "Invalid port"
    return value

def validate_path (value):
    if not value:
        raise 'No path'
    if value[0] == '/':
        return value
    if value[1:3] == ":\\":
        return value
    raise 'Broken path'

def validate_path_list (value):
    re = []
    for p in value.split(','):
        re.append(validate_path(p))
    return reduce(lambda x,y: x+','+y, re)

def validate_positive_int (value):
    tmp = int(value)
    if tmp < 0:
        raise "Negative"
    return value
