/*Arculator 2.2 by Sarah Walker
  Disc support*/
#include <stdio.h>
#include <string.h>
#include "zip.h"

#include "arc.h"
#include "config.h"

#include "disc.h"

#include "disc_adf.h"
#include "disc_apd.h"
#include "disc_fdi.h"
#include "disc_hfe.h"
#include "disc_jfd.h"
#include "disc_scp.h"

#include "ddnoise.h"

#include "ioc.h"
#include "timer.h"

char discname[4][512];
int defaultwriteprot = 0;
int disc_noise_gain;

static emu_timer_t disc_timer;

disc_funcs_t *drive_funcs[4];

int disc_drivesel = 0;
static int disc_notfound = 0;

int curdrive = 0;
int discchange[4];
int fdctype;
int readflash[4];
int motoron;
int writeprot[4];

static int disc_current_track[4] = {0, 0, 0, 0};

static uint64_t disc_poll_time;
static const int disc_poll_times[4] =
{
	32, /*Double density*/
	32, /*Double density*/
	16, /*High density*/
	8   /*Extended density - supported by SuperIO but never used on the Arc*/
};

fdc_funcs_t *fdc_funcs;
void *fdc_p;
emu_timer_t *fdc_timer;
int fdc_overridden;
int fdc_ready;

typedef struct
{
	char *ext;
	void (*load)(int drive, FILE *f, int fwriteprot);
	int size;
}
disc_loader_t;

static disc_loader_t loaders[]=
{
	{"SSD", ssd_load,         80*10* 256},
	{"DSD", dsd_load,       2*80*10* 256},
	{"ADF", adf_load,         80*16* 256},
	{"ADF", adf_arcdd_load, 2*80* 5*1024},
	{"ADF", adf_archd_load, 2*80*10*1024},
	{"ADL", adl_load,       2*80*16* 256},
	{"HFE", hfe_load,       -1},
#ifndef __EMSCRIPTEN__
	{"FDI", fdi_load,       -1},
	{"APD", apd_load,       -1},
	{"SCP", scp_load,       -1},
#endif
//        {"JFD", jfd_load,       -1},
	{0,0,0}
};

static disc_loader_t* disc_loader_for(char *filename, int size)
{
    char *ext = get_extension(filename);
    if (!ext)
        return NULL;

    for (int c=0; loaders[c].ext; c++)
    {
        if (!strcasecmp(ext, loaders[c].ext) && (size <= loaders[c].size || loaders[c].size == -1))
        {
            rpclog("Loading as %s\n", ext);
            return &loaders[c];
        }
    }
    return NULL;
    //        printf("Couldn't load %s %s\n",fn,p);
}

void disc_load(int drive, char *fn)
{
    disc_loader_t *loader = NULL;
	long long size;
	FILE *f, *fz;
    char sig[4];
    int fwriteprot = 0;

	rpclog("disc_load %i %s\n", drive, fn);
//        setejecttext(drive, "");
	if (!fn) return;
//        setejecttext(drive, fn);
	rpclog("Loading :%i %s\n", drive, fn);

	f = fopen(fn, "r+");
	if (!f) {
        f = fopen(fn, "r");
        if (!f) {
            rpclog("Couldn't open %s\n", fn);
            return;
        }
        fwriteprot = 1;
    }
    if (fread(sig, 1, 4, f) < 4 || fseek(f, -1, SEEK_END) < 0)
    {
        rpclog("Couldn't read/seek %s\n", fn);
        fclose(f);
        return;
    }
	size = ftell(f) + 1;

    if (memcmp(sig, "PK\3\4", 4) == 0)
    {
        struct zip_t* zip = zip_open(fn, 0, 'r');
        
        if (!zip) {
            rpclog("Couldn't open %s as zip\n", fn);
            return;
        }
        int entries = zip_entries_total(zip);
        for (int i = 0; i < entries && !loader; i++)
        {
            if (zip_entry_openbyindex(zip, i) < 0) {
                rpclog("Couldn't open %s as zip entry %i\n", fn, i);
                zip_close(zip);
                fclose(f);
                return;
            }
            if (zip_entry_isdir(zip) == 0) {
                const char *name2 = zip_entry_name(zip);
                unsigned long long size2 = zip_entry_size(zip);
                loader = disc_loader_for((char*) name2, size2);

                if (size2 > 10000000) {
                    rpclog("Skipping %s - too big to be a floppy\n", name2);
                    loader = NULL;
                }
            }
            zip_entry_close(zip);
        }


        if (loader) 
        {
            void *buf;
            size_t bufsize;
            rpclog("Found loader %s for %s\n", loader->ext, fn);
            zip_entry_read(zip, &buf, &bufsize);
            zip_close(zip);
            fclose(f);
            fz = fmemopen(buf, bufsize, "r");
            if (!fz) {
                rpclog("fmemopen failed?\n");
                return;
            }
        } 
        else 
        {
            rpclog("No loader found for any files in zip\n", fn);
            zip_close(zip);
            fclose(f);
            return;
        }
    }
    else
    {
        loader = disc_loader_for(fn, size);
        fz = f;
    }

    if (loader) {
        rpclog("Loading as %s\n", loader->ext);
        loader->load(drive, fz, fwriteprot);
        return;
    }

    /*No extension match, so guess based on image size*/
    rpclog("Size %i\n", size);
	if (size == (1680*1024)) /*1680k DOS - 80*2*21*512*/
	{
		adf_loadex(drive, fz, fwriteprot, 21, 512, 1, 0, 2, 1);
		return;
	}
	if (size == (1440*1024)) /*1440k DOS - 80*2*18*512*/
	{
        adf_loadex(drive, fz, fwriteprot, 18, 512, 1, 0, 2, 1);
        return;
	}
	if (size == (800*1024)) /*800k ADFS/DOS - 80*2*5*1024*/
	{
		loaders[2].load(drive, fz, fwriteprot);
		return;
	}
	if (size == (640*1024)) /*640k ADFS/DOS - 80*2*16*256*/
	{
        loaders[3].load(drive, fz, fwriteprot);
        return;
	}
	if (size == (720*1024)) /*720k DOS - 80*2*9*512*/
	{
        adf_loadex(drive, fz, fwriteprot, 9, 512, 1, 0, 1, 1);
        return;
	}
	if (size == (360*1024)) /*360k DOS - 40*2*9*512*/
	{
        adf_loadex(drive, fz, fwriteprot, 9, 512, 1, 1, 1, 1);
        return;
	}
	if (size <= (200 * 1024)) /*200k DFS - 80*1*10*256*/
	{
        loaders[0].load(drive, fz, fwriteprot);
        return;
	}
	if (size <= (400 * 1024)) /*400k DFS - 80*2*10*256*/
	{
        loaders[1].load(drive, fz, fwriteprot);
        return;
	}
    rpclog("Can't automatically decide for disc size %d, giving up", size);
    fclose(f);
    return;
}

void disc_new(int drive, char *fn)
{
	int c = 0, d;
	FILE *f;
	char *p = get_extension(fn);
	while (loaders[c].ext)
	{
		if (!strcasecmp(p, loaders[c].ext) && loaders[c].size != -1)
		{
			f=fopen(fn, "wb");
			for (d = 0; d < loaders[c].size; d++) putc(0, f);
			if (!strcasecmp(p, "ADF"))
			{
				fseek(f, 0, SEEK_SET);
				putc(7, f);
				fseek(f, 0xFD, SEEK_SET);
				putc(5, f); putc(0, f); putc(0xC, f); putc(0xF9, f); putc(0x04, f);
				fseek(f, 0x1FB, SEEK_SET);
				putc(0x88,f); putc(0x39,f); putc(0,f); putc(3,f); putc(0xC1,f);
				putc(0, f); putc('H', f); putc('u', f); putc('g', f); putc('o', f);
				fseek(f, 0x6CC, SEEK_SET);
				putc(0x24, f);
				fseek(f, 0x6D6, SEEK_SET);
				putc(2, f); putc(0, f); putc(0, f); putc(0x24, f);
				fseek(f, 0x6FB, SEEK_SET);
				putc('H', f); putc('u', f); putc('g', f); putc('o', f);
			}
			if (!strcasecmp(p, "ADL"))
			{
				fseek(f, 0, SEEK_SET);
				putc(7, f);
				fseek(f, 0xFD, SEEK_SET);
				putc(0xA, f); putc(0, f); putc(0x11, f); putc(0xF9, f); putc(0x09, f);
				fseek(f, 0x1FB, SEEK_SET);
				putc(0x01, f); putc(0x84, f); putc(0, f); putc(3, f); putc(0x8A, f);
				putc(0, f); putc('H', f); putc('u', f); putc('g', f); putc('o', f);
				fseek(f, 0x6CC, SEEK_SET);
				putc(0x24, f);
				fseek(f, 0x6D6, SEEK_SET);
				putc(2, f); putc(0, f); putc(0, f); putc(0x24, f);
				fseek(f, 0x6FB, SEEK_SET);
				putc('H', f); putc('u', f); putc('g', f); putc('o', f);
			}
			fclose(f);
			disc_load(drive, fn);
			return;
		}
		c++;
	}
}

void disc_close(int drive)
{
	rpclog("disc_close %i\n", drive);
	if (drive_funcs[drive])
	{
		drive_funcs[drive]->close(drive);
		drive_funcs[drive] = NULL;
	}
	disc_stop(drive);
}

int disc_empty(int drive)
{
	return !drive_funcs[drive];
}

void disc_init()
{
	memset(drive_funcs, 0, sizeof(drive_funcs));
}

void disc_reset()
{
	curdrive = 0;
	timer_add(&disc_timer, disc_poll, NULL, 0);
}

void disc_poll(void *p)
{
	if (drive_funcs[disc_drivesel] && drive_funcs[disc_drivesel]->high_res_poll)
		timer_advance_u64(&disc_timer, disc_poll_time >> 4);
	else
		timer_advance_u64(&disc_timer, disc_poll_time);
	if (drive_funcs[disc_drivesel])
	{
		if (drive_funcs[disc_drivesel]->poll)
			drive_funcs[disc_drivesel]->poll();
		if (disc_notfound)
		{
			disc_notfound--;
			if (!disc_notfound)
				fdc_funcs->notfound(fdc_p);
		}
	}
}

int disc_get_current_track(int drive)
{
	return disc_current_track[drive];
}

void disc_seek(int drive, int new_track)
{
	if (drive_funcs[drive] && drive_funcs[drive]->seek)
		drive_funcs[drive]->seek(drive, new_track);
	if (new_track != disc_current_track[drive] && !disc_empty(drive))
		ioc_discchange_clear(drive);
	ddnoise_seek(new_track - disc_current_track[drive]);
	disc_current_track[drive] = new_track;
}

void disc_readsector(int drive, int sector, int track, int side, int density)
{
	if (drive_funcs[drive] && drive_funcs[drive]->readsector)
		drive_funcs[drive]->readsector(drive, sector, track, side, density);
	else
		disc_notfound = 10000;
}

void disc_writesector(int drive, int sector, int track, int side, int density)
{
	if (drive_funcs[drive] && drive_funcs[drive]->writesector)
		drive_funcs[drive]->writesector(drive, sector, track, side, density);
	else
		disc_notfound = 10000;
}

void disc_readaddress(int drive, int track, int side, int density)
{
	if (drive_funcs[drive] && drive_funcs[drive]->readaddress)
		drive_funcs[drive]->readaddress(drive, track, side, density);
	else
		disc_notfound = 10000;
}

void disc_format(int drive, int track, int side, int density)
{
	if (drive_funcs[drive] && drive_funcs[drive]->format)
		drive_funcs[drive]->format(drive, track, side, density);
	else
		disc_notfound = 10000;
}

void disc_stop(int drive)
{
	if (drive_funcs[drive] && drive_funcs[drive]->stop)
		drive_funcs[drive]->stop();
}

void disc_set_motor(int enable)
{
	if (!enable)
		timer_disable(&disc_timer);
	else if (!timer_is_enabled(&disc_timer))
		timer_set_delay_u64(&disc_timer, disc_poll_time);
}

void disc_set_density(int density)
{
	disc_poll_time = (disc_poll_times[density] * TIMER_USEC);
}
