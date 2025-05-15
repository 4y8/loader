#include "bootloader.h"

#include "ata2.h"
#include "fb.h"
#include "console.h"
#include "keypad.h"
#include "minilibc.h"
#include "ipodhw.h"
#include "vfs.h"
#include "interrupts.h"

#include "../libc/u.h"
#include "../a3gunix/vm.h"

uint16 *framebuffer;
static int orig_contrast;

static void shutdown_loader (void)
{
  keypad_exit ();
  ata_exit ();
  exit_irqs ();
}

static void standby (void)
{
  shutdown_loader ();
  ipod_set_backlight (0);
  fb_cls (framebuffer, ipod_get_hwinfo()->lcd_is_grayscale?BLACK:WHITE);
  fb_update(framebuffer);
  mlc_delay_ms (1000);
  pcf_standby_mode ();
}

/*
void
launch_vm(void)
{
	int fd;
	struct vm vm;
	u32 fsize, read;
	u32 *init;

	fd = vfs_open("(hd0,1)/init");
	vfs_seek(fd, 0, VFS_SEEK_END);
	fsize = vfs_tell(fd);
	vfs_seek(fd, 0, VFS_SEEK_SET);
	init = mlc_malloc(fsize);
	read = 0;
	while (fsize > read * 4) {
		if (fsize - read * 4 > 128 * 1024) {
			vfs_read((void *)(init + read), 128 * 1024, 1, fd);
			read += 128 * 1024 / 4;
		} else {
			vfs_read((void *)(init + read), fsize - read * 4, 1, fd);
			read = fsize >> 2;
		}
	}
	mlc_printf("VM\n");

	vm.pc = 0;
	vm.code = (struct instruction *)init;
	vm_run(&vm);
}*/

void
launch_vm(void)
{
	struct vm vm;

	vm_load_file("(hd0,1)/init", &vm);
	vm_run(&vm);
}

// -----------------------
//  main entry of loader2
// -----------------------

void *loader(void) {
  uint32 ret;
  ipod_t *ipod;

  ipod_init_hardware();
  ipod = ipod_get_hwinfo();
  mlc_malloc_init();

  mlc_set_output_options (1, 0);  // this caches screen text output for now

  init_irqs (); // basic intr initialization - does not enable IRQs yet
  
  framebuffer = (uint16*)mlc_malloc( ipod->lcd_width * ipod->lcd_height * 2 );
  fb_init();
  fb_cls(framebuffer, BLACK);
  fb_update (framebuffer);

  orig_contrast = lcd_curr_contrast();
  if (ipod->lcd_is_grayscale && ipod->hw_ver >= 3) {
    // increase the contrast a little on 3G, 4G and Minis because of their crappy LCDs
    // whose contrast weakens with certain patterns (e.g. horizontal lines as they appear
    // in the menu's frame)
    lcd_set_contrast (orig_contrast + 4);
  }

  console_init(framebuffer);

  keypad_init();

  // use this to test for keys held down at startup:
  uint8 startup_keys = keypad_getstate ();
  if (startup_keys) {
    mlc_printf("keys: %x\n", startup_keys);
    if (startup_keys & IPOD_KEYPAD_PREV) {
      // Rewind is held down at start
    }
  }

  ret = ata_init();
  if (ret) {
    mlc_printf("ATAinit: %i\n",ret);
    mlc_show_fatal_error ();
  }

  ata_identify();
  vfs_init();

  enable_irqs ();

  keypad_flush (); // discard buttons that were already pressed at start

  mlc_clear_screen ();

  fb_update(framebuffer);

  launch_vm();
  
  mlc_printf("goodbye\n");
  mlc_delay_ms(1000);
  standby();
}
