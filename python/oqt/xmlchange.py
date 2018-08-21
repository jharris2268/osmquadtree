import time, calendar

def read_timestamp(ts):

    try:
        return calendar.timegm(time.strptime(ts[:19],'%Y-%m-%dT%H:%M:%S'))
    except:
        pass
    try:
        return calendar.timegm(time.strptime(ts[:19],'%Y-%m-%dT%H-%M-%S'))
    except:
        pass
    raise Exception("time '%s' in neither format '%Y-%m-%dT%H:%M:%S' or '%Y-%m-%dT%H-%M-%S'" % ts)


