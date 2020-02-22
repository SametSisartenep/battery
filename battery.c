#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <draw.h>
#include <event.h>

enum
{
	SEC = 1000,	/* ms */

	/* battery discharge levels and charging state */
	JLOW = 0,
	JMED,
	JHIG,
	JCHR,
	NBATTLVL,

	/* percentage text colors */
	PDIS = 0,
	PCHR,
	NPERC,

	/* color palette */
	CBG = 0,
	CFG,
	CJUICE,
	NCOLOR,

	Labspace = 2	/* room around the label */
};

Image *juicecolors[NBATTLVL], *textcolors[NPERC], *palette[NCOLOR];
int bfd, nbatt, lvl, ischarging;
char lvlstr[5];

char battfile[] = "/mnt/acpi/battery";

void
juicereflow(double ratio)
{
	Point sp;

	sp = Pt(screen->r.min.x, screen->r.max.y - Dy(screen->r)*ratio);
	draw(screen, Rpt(sp, screen->r.max), palette[CJUICE], nil, ZP);
}

void
label(Point p, char *text)
{
	string(screen, p, palette[CFG], ZP, font, text);
}

Image *
rgba(ulong c)
{
	Image *i;

	i = allocimage(display, Rect(0,0,1,1), screen->chan, 1, c);
	if(i == nil)
		sysfatal("allocimage: %r");
	return i;
}

void
redraw(void)
{
	Point p = Pt(((screen->r.min.x+screen->r.max.x)/2)-((font->width*strlen(lvlstr))/2),
		screen->r.max.y-(font->height+Labspace));

	draw(screen, screen->r, palette[CBG], nil, ZP);
	juicereflow(lvl/100.0);
	label(p, lvlstr);
}

void
update(void)
{
	char buf[256], idbuf[16], *s, *p;
	int n;

	if((n = pread(bfd, buf, sizeof(buf)-1, 0)) <= 0)
		sysfatal("pread: %r");
	buf[n] = 0;
	s = buf;
	nbatt = 0;
	lvl = strtol(s, &s, 0);
	do{
		if(isalpha(*s)){
			p = idbuf;
			while(isalpha(*s) && p < idbuf + sizeof idbuf - 1)
				*p++ = *s++;
			*p = 0;
			if(strcmp(idbuf, "charging") == 0)
				ischarging = 1;
			else if(strcmp(idbuf, "discharging") == 0)
				ischarging = 0;
		}
		if(*s == '\n'){
			nbatt++;
			if(isdigit(*++s))
				lvl += strtol(s, &s, 0);
		}
	}while(*s++);
	lvl /= nbatt;
	if(ischarging){
		palette[CFG] = textcolors[PCHR];
		palette[CJUICE] = juicecolors[JCHR];
	}else{
		palette[CFG] = textcolors[PDIS];
		palette[CJUICE] = lvl < 20 ? juicecolors[JLOW]:
				lvl < 60 ? juicecolors[JMED]:
					juicecolors[JHIG];
	}
	snprint(lvlstr, sizeof lvlstr, "%d%%", lvl);
}

void
eresized(int new)
{
	if(new && getwindow(display, Refnone) < 0)
		fprint(2, "can't reattach to window\n");
	redraw();
}

void
usage(void)
{
	fprint(2, "usage: %s\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	Event e;
	int Etimer, key;

	ARGBEGIN{
	default: usage();
	}ARGEND;
	if(argc != 0)
		usage();
	bfd = open(battfile, OREAD);
	if(bfd < 0)
		sysfatal("open: %r");
	if(initdraw(0, 0, "battery") < 0)
		sysfatal("initdraw: %r");
	juicecolors[JLOW] = rgba(DRed);
	juicecolors[JMED] = rgba(DDarkyellow);
	juicecolors[JHIG] = rgba(DMedgreen);
	juicecolors[JCHR] = rgba(0x0088CCFF);
	textcolors[PDIS] = display->black;
	textcolors[PCHR] = rgba(DYellow);
	palette[CBG] = allocimagemix(display, 0x88FF88FF, DWhite);
	palette[CFG] = textcolors[PDIS];
	einit(Emouse);
	Etimer = etimer(0, 120*SEC);
	update();
	redraw();
	for(;;){
		key = event(&e);
		if(key == Etimer)
			update();
		redraw();
	}
}
