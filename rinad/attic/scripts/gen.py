import re

# 'JAVA_FA_ABBASTANZA_XXX' --> 'JavaFaAbbastanzaXxx'
def javize(s):
    ret = '';
    uscore = False;

    for i in range(len(s)):
        if len(ret) == 0:
            ret += s[i].upper();
        elif s[i] == '_':
            uscore = True;
        elif uscore:
            ret += s[i].upper();
            uscore = False;
        else:
            ret += s[i].lower();

    return ret



out1 = '';
out2 = '';

# input contains a string like 'AAA_BBB_CCC_DDD' per line
# no spaces allowed
f = open('t', 'r');
while 1:
    line = f.readline();
    if line == '':
        break

    line = line.replace('\n', '');
    name1 = line;
    name2 = javize(name1);
    name2.replace('Ipc', 'IPC');
    name2 += 'Handler';

    #print("'%s' '%s'" % (name1, name2));

    template1 = "static void %s(rina::IPCEvent *event)\n{\n}\n\n";
    template2 = "loop.register_event(rina::%s,\n\t\t\t\t%s);\n";

    out1 += template1 % (name2)
    out2 += template2 % (name1, name2)

print(out1)
print(out2)
