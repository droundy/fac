
import re, string, os, glob
import markdown as mmdd

import xml.etree.ElementTree as ET


def mkdown(mdfile):
    htfile = mdfile[:-2]+'html'

    f = open(mdfile, 'r')
    mkstr = f.read()
    f.close()

    f = open('web/sidebar.md', 'r')
    sidebar = f.read()
    f.close()

    f = open('web/docnav.md', 'r')
    docnav = '<nav class="docnav">'+mmdd.markdown(f.read(), extensions=['def_list'])+'</nav>'
    f.close()

    f = open('web/template.html', 'r')
    templatestr = f.read()
    f.close()

    titlere = re.compile(r"^\s*#\s*([^\n]*)(.*)", re.DOTALL)
    title = titlere.findall(mkstr)
    if len(title) == 0:
        title = "Fac"
    else:
        mkstr = title[0][1]
        title = mmdd.markdown(title[0][0])
        title = title[3:len(title)-4]
    if title[:4] == "Fac ":
        pagetitle = title[4:]
    else:
        pagetitle = title

    if '$docnav' in mkstr:
        templatestr = templatestr.replace('$docnav', docnav)
        mkstr = mkstr.replace('$docnav', '')
    else:
        templatestr = templatestr.replace('$docnav', '')

    template = string.Template(templatestr)

    f = open(htfile, 'w')
    myhtml = template.safe_substitute(
            title = title,
            pagetitle = pagetitle,
            #content = mmdd.markdown(mkstr, extensions=['mathjax']),
            #sidebar = mmdd.markdown(sidebar, extensions=['mathjax'])))
            content = mmdd.markdown(mkstr, extensions=['def_list']),
            sidebar = mmdd.markdown(sidebar, extensions=['def_list']))
    myhtml = myhtml.replace('<li><a href="'+mdfile[4:-3],
                            '<li><a class="current" href="'+mdfile[4:-3])

    ff = open('temp.html', 'w')
    ff.write(myhtml)
    ff.close()
    etree = ET.fromstring(myhtml)
    for main in etree.iter('article'):
        lastheader = None
        for p in list(main):
            if p.tag == 'h2' or lastheader is None:
                lastheader = ET.SubElement(main, 'div', {'class': 'indivisible'})
                lastheader.append(p)
                main.remove(p)
            elif lastheader is not None:
                lastheader.append(p)
                main.remove(p)
        #f.write('XXX '+ET.tostring(main, encoding='utf-8', method='html')+'\n')
        #etree.remove(main)

    f.write(ET.tostring(etree, encoding='unicode', method='html'))

    #f.write(myhtml)
    f.close()

for f in glob.glob('web/*.md'):
    mkdown(f)
