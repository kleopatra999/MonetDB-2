import os, sys, subprocess


def main():
	TSTTRGDIR = os.environ['TSTTRGDIR']
	SQLCLIENT = os.environ['SQLCLIENT']
	MAPIPORT = os.environ['MAPIPORT']

        cmd = str('%s -p %s' % (SQLCLIENT, MAPIPORT))
	f = open(os.path.join(TSTTRGDIR, 'dumpoutput.sql'), 'r')
	clt = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.PIPE)
	clt.stdin.write(f.read())	
	clt.stdin.close()
	f.close()
	sys.stdout.write(clt.stdout.read())
	clt.stdout.close()
	sys.stderr.write(clt.stderr.read())
	clt.stderr.close()
	

main()
