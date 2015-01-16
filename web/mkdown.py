
import re, string, os
import markdown as mmdd


toupload = set()
def Upload(env, source):
    source = str(source)
    global toupload
    toupload.add('.layout/style.css')
    toupload.add('papers/pair-correlation/figs/pretty-4.svg') # background
    toupload.add(source)
    Depends('upload', source)

def doupload(target, source, env):
    host = 'science.oregonstate.edu'
    path = 'public_html/deft'
    os.system('ssh %s rm -rf %s' % (host, path))
    madedir = set()
    for f in toupload:
        dirname = os.path.join(path, os.path.dirname(f))
        if not dirname in madedir:
            print 'creating directory', dirname
            os.system('ssh %s mkdir -p %s' % (host, dirname))
            madedir.add(dirname)
        os.system('scp %s %s:%s/' % (f, host, os.path.join(path, os.path.dirname(f))))

def mkdown(mdfile):
    htfile = mdfile[:-2]+'html'

    f = open(mdfile, 'r')
    mkstr = f.read()
    f.close()

    f = open('web/sidebar.md', 'r')
    sidebarstr = f.read()
    f.close()

    f = open('web/template.html', 'r')
    templatestr = f.read()
    f.close()

    titlere = re.compile(r"^\s*#\s*([^\n]*)(.*)", re.DOTALL)
    title = titlere.findall(mkstr)
    if len(title) == 0:
        title = "Bilge?"
    else:
        mkstr = title[0][1]
        title = mmdd.markdown(title[0][0])
        title = title[3:len(title)-4]
    sidebar = sidebarstr
    template = string.Template(templatestr)

    f = open(htfile, 'w')
    f.write(template.safe_substitute(
            title = title,
            #content = mmdd.markdown(mkstr, extensions=['mathjax']),
            #sidebar = mmdd.markdown(sidebar, extensions=['mathjax'])))
            content = mmdd.markdown(mkstr),
            sidebar = mmdd.markdown(sidebar)))
    f.close()

mkdown('web/index.md')
mkdown('web/documentation.md')
