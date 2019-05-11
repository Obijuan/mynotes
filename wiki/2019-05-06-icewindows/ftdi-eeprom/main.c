/***************************************************************************
                             main.c  -  description
                           -------------------
    begin                : Mon Apr  7 12:05:22 CEST 2003
    copyright            : (C) 2003-2014 by Intra2net AG and the libftdi developers
    email                : opensource@intra2net.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 ***************************************************************************/

/*
 TODO:
    - Merge Uwe's eeprom tool. Current features:
        - Init eeprom defaults based upon eeprom type
        - Read -> Already there
        - Write -> Already there
        - Erase -> Already there
        - Decode on stdout
        - Ability to find device by PID/VID, product name or serial

 TODO nice-to-have:
    - Out-of-the-box compatibility with FTDI's eeprom tool configuration files
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include <confuse.h>
#include <libusb.h>
#include <ftdi.h>
#include <ftdi_eeprom_version.h>

static int parse_cbus(cfg_t *cfg, cfg_opt_t *opt, const char *value, void *result)
{
    static const char* options[] =
    {
        "TXDEN", "PWREN", "RXLED", "TXLED", "TXRXLED", "SLEEP", "CLK48",
        "CLK24", "CLK12", "CLK6", "IOMODE", "BB_WR", "BB_RD"
    };

    int i;
    for (i=0; i<sizeof(options)/sizeof(*options); i++)
    {
        if (!(strcmp(options[i], value)))
        {
            *(int *)result = i;
            return 0;
        }
    }

    cfg_error(cfg, "Invalid %s option '%s'", cfg_opt_name(opt), value);
    return -1;
}

static int parse_cbush(cfg_t *cfg, cfg_opt_t *opt, const char *value, void *result)
{
    static const char* options[] =
    {
        "TRISTATE", "TXLED", "RXLED", "TXRXLED", "PWREN", "SLEEP",
        "DRIVE_0", "DRIVE1", "IOMODE", "TXDEN", "CLK30", "CLK15", "CLK7_5"
    };

    int i;
    for (i=0; i<sizeof(options)/sizeof(*options); i++)
    {
        if (!(strcmp(options[i], value)))
        {
            *(int *)result = i;
            return 0;
        }
    }

    cfg_error(cfg, "Invalid %s option '%s'", cfg_opt_name(opt), value);
    return -1;
}

static int parse_cbusx(cfg_t *cfg, cfg_opt_t *opt, const char *value, void *result)
{
    static const char* options[] =
    {
        "TRISTATE", "TXLED", "RXLED", "TXRXLED", "PWREN", "SLEEP",
        "DRIVE_0", "DRIVE1", "IOMODE", "TXDEN", "CLK24", "CLK12",
        "CLK6", "BAT_DETECT", "BAT_DETECT_NEG", "I2C_TXE", "I2C_RXF", "VBUS_SENSE",
        "BB_WR", "BB_RD", "TIME_STAMP", "AWAKE"
    };

    int i;
    for (i=0; i<sizeof(options)/sizeof(*options); i++)
    {
        if (!(strcmp(options[i], value)))
        {
            *(int *)result = i;
            return 0;
        }
    }

    cfg_error(cfg, "Invalid %s option '%s'", cfg_opt_name(opt), value);
    return -1;
}

static int parse_chtype(cfg_t *cfg, cfg_opt_t *opt, const char *value, void *result)
{
    static const struct
    {
        char* key;
	int   opt;
    } options[] =
    {
        { "UART",   CHANNEL_IS_UART },
        { "FIFO",   CHANNEL_IS_FIFO },
        { "OPTO",   CHANNEL_IS_OPTO },
        { "CPU",    CHANNEL_IS_CPU },
        { "FT1284", CHANNEL_IS_FT1284}
    };

    int i;
    for (i=0; i<sizeof(options)/sizeof(*options); i++)
    {
        if (!(strcmp(options[i].key, value)))
        {
            *(int *)result = options[i].opt;
            return 0;
        }
    }

    cfg_error(cfg, "Invalid %s option '%s'", cfg_opt_name(opt), value);
    return -1;
}

/**
 * @brief Set eeprom value
 *
 * \param ftdi pointer to ftdi_context
 * \param value_name Enum of the value to set
 * \param value Value to set
 *
 * Function will abort the program on error
 **/
static void eeprom_set_value(struct ftdi_context *ftdi, enum ftdi_eeprom_value value_name, int value)
{
    if (ftdi_set_eeprom_value(ftdi, value_name, value) < 0)
    {
        printf("Unable to set eeprom value %d: %s. Aborting\n", value_name, ftdi_get_error_string(ftdi));
        exit (-1);
    }
}

/**
 * @brief Get eeprom value
 *
 * \param ftdi pointer to ftdi_context
 * \param value_name Enum of the value to get
 * \param value Value to get
 *
 * Function will abort the program on error
 **/
static void eeprom_get_value(struct ftdi_context *ftdi, enum ftdi_eeprom_value value_name, int *value)
{
    if (ftdi_get_eeprom_value(ftdi, value_name, value) < 0)
    {
        printf("Unable to get eeprom value %d: %s. Aborting\n", value_name, ftdi_get_error_string(ftdi));
        exit (-1);
    }
}

static void eeprom_get_string(struct ftdi_context *ftdi, char *manuf, char *product)
{
    if (ftdi_eeprom_get_strings(ftdi, manuf, 50, product, 50, NULL, 50) < 0)
    {
        printf("ERROR: %s", ftdi_get_error_string(ftdi));
        exit (-1);
    }
    
}

static void usage(const char *program)
{
    fprintf(stderr, "Syntax: %s [...options...] <config-file>\n", program);
    fprintf(stderr, "Valid Options:\n");
    fprintf(stderr, "--device <description>  Specify device to open by description string. One of:\n");
    fprintf(stderr, "         d:<devicenode>\n");
    fprintf(stderr, "         i:<vendor>:<product>\n");
    fprintf(stderr, "         i:<vendor>:<product>:<index>\n");
    fprintf(stderr, "         s:<vendor>:<product>:<serial>\n");
    fprintf(stderr, "--read-eeprom           Read eeprom and write to -filename- from config-file\n");
    fprintf(stderr, "--build-eeprom          Build eeprom image\n");
    fprintf(stderr, "--erase-eeprom          Erase eeprom\n");
    fprintf(stderr, "--flash-eeprom          Flash eeprom\n");
}

int main(int argc, char *argv[])
{
    /*
    configuration options
    */
    cfg_opt_t opts[] =
    {
        CFG_INT("vendor_id", 0, 0),
        CFG_INT("product_id", 0, 0),
        CFG_BOOL("self_powered", cfg_true, 0),
        CFG_BOOL("remote_wakeup", cfg_true, 0),
        CFG_BOOL("in_is_isochronous", cfg_false, 0),
        CFG_BOOL("out_is_isochronous", cfg_false, 0),
        CFG_BOOL("suspend_pull_downs", cfg_false, 0),
        CFG_BOOL("use_serial", cfg_false, 0),
        CFG_BOOL("change_usb_version", cfg_false, 0),
        CFG_INT("usb_version", 0, 0),
        CFG_INT("max_power", 0, 0),
        CFG_STR("manufacturer", "Acme Inc.", 0),
        CFG_STR("product", "USB Serial Converter", 0),
        CFG_STR("serial", "08-15", 0),
        CFG_INT("default_pid", 0x6001, 0),
        CFG_INT("eeprom_type", 0x00, 0),
        CFG_STR("filename", "", 0),
        CFG_BOOL("flash_raw", cfg_false, 0),
        CFG_BOOL("high_current", cfg_false, 0),


        CFG_INT_CB("cbus0", -1, 0, parse_cbus),
        CFG_INT_CB("cbus1", -1, 0, parse_cbus),
        CFG_INT_CB("cbus2", -1, 0, parse_cbus),
        CFG_INT_CB("cbus3", -1, 0, parse_cbus),
        CFG_INT_CB("cbus4", -1, 0, parse_cbus),
        CFG_INT_CB("cbush0", -1, 0, parse_cbush),
        CFG_INT_CB("cbush1", -1, 0, parse_cbush),
        CFG_INT_CB("cbush2", -1, 0, parse_cbush),
        CFG_INT_CB("cbush3", -1, 0, parse_cbush),
        CFG_INT_CB("cbush4", -1, 0, parse_cbush),
        CFG_INT_CB("cbush5", -1, 0, parse_cbush),
        CFG_INT_CB("cbush6", -1, 0, parse_cbush),
        CFG_INT_CB("cbush7", -1, 0, parse_cbush),
        CFG_INT_CB("cbush8", -1, 0, parse_cbush),
        CFG_INT_CB("cbush9", -1, 0, parse_cbush),
        CFG_INT_CB("cbusx0", -1, 0, parse_cbusx),
        CFG_INT_CB("cbusx1", -1, 0, parse_cbusx),
        CFG_INT_CB("cbusx2", -1, 0, parse_cbusx),
        CFG_INT_CB("cbusx3", -1, 0, parse_cbusx),
        CFG_BOOL("invert_txd", cfg_false, 0),
        CFG_BOOL("invert_rxd", cfg_false, 0),
        CFG_BOOL("invert_rts", cfg_false, 0),
        CFG_BOOL("invert_cts", cfg_false, 0),
        CFG_BOOL("invert_dtr", cfg_false, 0),
        CFG_BOOL("invert_dsr", cfg_false, 0),
        CFG_BOOL("invert_dcd", cfg_false, 0),
        CFG_BOOL("invert_ri", cfg_false, 0),
        CFG_INT_CB("cha_type", -1, 0, parse_chtype),
        CFG_INT_CB("chb_type", -1, 0, parse_chtype),
        CFG_BOOL("cha_vcp", cfg_true, 0),
        CFG_BOOL("chb_vcp", cfg_true, 0),
        CFG_BOOL("chc_vcp", cfg_true, 0),
        CFG_BOOL("chd_vcp", cfg_true, 0),
        CFG_BOOL("cha_rs485", cfg_false, 0),
        CFG_BOOL("chb_rs485", cfg_false, 0),
        CFG_BOOL("chc_rs485", cfg_false, 0),
        CFG_BOOL("chd_rs485", cfg_false, 0),
        CFG_FUNC("include", &cfg_include),
        CFG_INT("user_data_addr", 0x18, 0),
        CFG_STR("user_data_file", "", 0),
        CFG_END()
    };
    cfg_t *cfg;

    /*
    normal variables
    */
    enum {
        COMMAND_READ = 1,
        COMMAND_ERASE,
        COMMAND_FLASH,
        COMMAND_BUILD
    } command = 0;
    const char *cfg_filename = NULL;
    const char *device_description = NULL;
    const char *user_data_file = NULL;
    char *user_data_buffer = NULL;

    const int max_eeprom_size = 256;
    int my_eeprom_size = 0;
    unsigned char *eeprom_buf = NULL;
    char *filename;
    int size_check;
    int i;
    FILE *fp;

    struct ftdi_context *ftdi = NULL;

    printf("\nFTDI eeprom generator v%s\n", EEPROM_VERSION_STRING);
    printf ("(c) Intra2net AG and the libftdi developers <opensource@intra2net.com>\n");

    for (i = 1; i < argc; i++) {
        if (*argv[i] != '-')
        {
            cfg_filename = argv[i];
        }
        else if (!strcmp(argv[i], "--device"))
        {
            if (i+1 >= argc)
            {
                usage(argv[0]);
                exit(-1);
            }
            device_description = argv[++i];
        }
        else if (!strcmp(argv[i], "--read-eeprom"))
        {
            command = COMMAND_READ;
        }
        else if (!strcmp(argv[i], "--erase-eeprom"))
        {
            command = COMMAND_ERASE;
        }
        else if (!strcmp(argv[i], "--flash-eeprom"))
        {
            command = COMMAND_FLASH;
        }
        else if (!strcmp(argv[i], "--build-eeprom"))
        {
            command = COMMAND_BUILD;
        }
        else
        {
            usage(argv[0]);
            exit(-1);
        }
    }

    if (!cfg_filename)
    {
        usage(argv[0]);
        exit(-1);
    }

    if ((fp = fopen(cfg_filename, "r")) == NULL)
    {
        printf ("Can't open configuration file\n");
        exit (-1);
    }
    fclose (fp);

    cfg = cfg_init(opts, 0);
    cfg_parse(cfg, cfg_filename);
    filename = cfg_getstr(cfg, "filename");

    if (cfg_getbool(cfg, "self_powered") && cfg_getint(cfg, "max_power") > 0)
        printf("Hint: Self powered devices should have a max_power setting of 0.\n");

    if ((ftdi = ftdi_new()) == 0)
    {
        fprintf(stderr, "Failed to allocate ftdi structure :%s \n",
                ftdi_get_error_string(ftdi));
        return EXIT_FAILURE;
    }

    if (device_description != NULL)
    {
        i = ftdi_usb_open_string(ftdi, device_description);

        if (i != 0)
        {
            printf("Unable to find FTDI device with description: %s\n",
                   device_description);
            printf("Error code: %d (%s)\n", i, ftdi_get_error_string(ftdi));
            exit (-1);
        }
    }
    else if (command > 0)
    {
        int vendor_id = cfg_getint(cfg, "vendor_id");
        int product_id = cfg_getint(cfg, "product_id");

        i = ftdi_usb_open(ftdi, vendor_id, product_id);

        if (i != 0)
        {
            int default_pid = cfg_getint(cfg, "default_pid");
            printf("Unable to find FTDI devices under given vendor/product id: 0x%X/0x%X\n", vendor_id, product_id);
            printf("Error code: %d (%s)\n", i, ftdi_get_error_string(ftdi));
            printf("Retrying with default FTDI pid=%#04x.\n", default_pid);

            i = ftdi_usb_open(ftdi, 0x0403, default_pid);
            if (i != 0)
            {
                printf("Error: %s\n", ftdi->error_str);
                exit (-1);
            }
        }
    }
    ftdi_eeprom_initdefaults (ftdi, cfg_getstr(cfg, "manufacturer"),
                              cfg_getstr(cfg, "product"),
                              cfg_getstr(cfg, "serial"));

    printf("FTDI read eeprom: %d\n", ftdi_read_eeprom(ftdi));
    eeprom_get_value(ftdi, CHIP_SIZE, &my_eeprom_size);
    printf("EEPROM size: %d\n", my_eeprom_size);

    if (command == COMMAND_READ)
    {
        ftdi_eeprom_decode(ftdi, 0 /* debug: 1 */);

        eeprom_buf = malloc(my_eeprom_size);
        ftdi_get_eeprom_buf(ftdi, eeprom_buf, my_eeprom_size);

        if (eeprom_buf == NULL)
        {
            fprintf(stderr, "Malloc failed, aborting\n");
            goto cleanup;
        }
        if (filename != NULL && strlen(filename) > 0)
        {
            FILE *fp = fopen (filename, "wb");
            
            if(fp)
            {
                fwrite(eeprom_buf, 1, my_eeprom_size, fp);
                fclose(fp);
            }
            else
                fprintf(stderr, "Could not open output file %s: %s\n", filename, strerror(errno));
        }
        else
        {
            printf("Warning: Not writing eeprom, you must supply a valid filename\n");
        }

        int vid;
	int pid;
	int self_powered;
	int remote_wakeup;
	int is_not_pnp;
	int suspend_dbus7; 
	int max_power;
	int in_is_isochronous;
	int out_is_isochronous;
	int suspend_pull_downs;
	int use_serial;
	int use_usb_version;
	int usb_version;
	char manufacturer[254];
	char product[254];
	char serial[254];
	int channel_a_type;
	int channel_b_type;
	int channel_a_driver;
	int channel_b_driver;
	int channel_c_driver;
	int channel_d_driver;
	int high_current;
	int high_current_a;
	int high_current_b;
	int cbus0,cbus1,cbus2,cbus3,cbus4;
	int cbush5, cbush6, cbush7, cbush8, cbush9;
	int invert;
	int channel_a_rs485;
	int channel_b_rs485;
	int channel_c_rs485;
	int channel_d_rs485;
	int user_data_addr;
	int group0_drive;
	int group0_schmitt; 
	int group0_slew;
	int group1_drive;
	int group1_schmitt; 
	int group1_slew;
	int group2_drive;
	int group2_schmitt; 
	int group2_slew;
	int group3_drive;
	int group3_schmitt; 
	int group3_slew;
	int eeprom_size;
	int eeprom_type;

	int power_save;
	int clock_polarity;
	int data_order;
	int flow_control;
	int release_number;
	int external_oscillator;

        eeprom_get_value(ftdi, VENDOR_ID, &vid);
        eeprom_get_value(ftdi, PRODUCT_ID, &pid);
	eeprom_get_value(ftdi, SELF_POWERED, &self_powered);
	eeprom_get_value(ftdi, REMOTE_WAKEUP, &remote_wakeup); 
	eeprom_get_value(ftdi, IS_NOT_PNP, &is_not_pnp);
	eeprom_get_value(ftdi, SUSPEND_DBUS7, &suspend_dbus7);
	eeprom_get_value(ftdi, IN_IS_ISOCHRONOUS, &in_is_isochronous);
	eeprom_get_value(ftdi, OUT_IS_ISOCHRONOUS, &out_is_isochronous);
	eeprom_get_value(ftdi, SUSPEND_PULL_DOWNS, &suspend_pull_downs);
	eeprom_get_value(ftdi, USE_SERIAL , &use_serial);
	eeprom_get_value(ftdi, USB_VERSION, &usb_version);
	eeprom_get_value(ftdi, USE_USB_VERSION, &use_usb_version);
	eeprom_get_value(ftdi, MAX_POWER, &max_power);	
	eeprom_get_value(ftdi, CHANNEL_A_TYPE, &channel_a_type);
	eeprom_get_value(ftdi, CHANNEL_B_TYPE, &channel_b_type);
	eeprom_get_value(ftdi, CHANNEL_A_DRIVER, &channel_a_driver);
	eeprom_get_value(ftdi, CHANNEL_B_DRIVER, &channel_b_driver);
	eeprom_get_value(ftdi, CBUS_FUNCTION_0, &cbus0);
	eeprom_get_value(ftdi, CBUS_FUNCTION_1, &cbus1);
	eeprom_get_value(ftdi, CBUS_FUNCTION_2, &cbus2);
	eeprom_get_value(ftdi, CBUS_FUNCTION_3, &cbus3);
	eeprom_get_value(ftdi, CBUS_FUNCTION_4, &cbus4);
	eeprom_get_value(ftdi, CBUS_FUNCTION_5, &cbush5);
	eeprom_get_value(ftdi, CBUS_FUNCTION_6, &cbush6);
	eeprom_get_value(ftdi, CBUS_FUNCTION_7, &cbush7);
	eeprom_get_value(ftdi, CBUS_FUNCTION_8, &cbush8);
	eeprom_get_value(ftdi, CBUS_FUNCTION_9, &cbush9);
	eeprom_get_value(ftdi, HIGH_CURRENT, &high_current);
	eeprom_get_value(ftdi, HIGH_CURRENT_A, &high_current_a);
	eeprom_get_value(ftdi, HIGH_CURRENT_B, &high_current_b);
	eeprom_get_value(ftdi, INVERT, &invert);
	eeprom_get_value(ftdi, GROUP0_DRIVE, &group0_drive);
	eeprom_get_value(ftdi, GROUP0_SCHMITT, &group0_schmitt);
	eeprom_get_value(ftdi, GROUP0_SLEW, &group0_slew);
	eeprom_get_value(ftdi, GROUP1_DRIVE, &group1_drive);
	eeprom_get_value(ftdi, GROUP1_SCHMITT, &group1_schmitt);
	eeprom_get_value(ftdi, GROUP1_SLEW, &group1_slew);
	eeprom_get_value(ftdi, GROUP2_DRIVE, &group2_drive);
	eeprom_get_value(ftdi, GROUP2_SCHMITT, &group2_schmitt);
	eeprom_get_value(ftdi, GROUP2_SLEW, &group2_slew);
	eeprom_get_value(ftdi, GROUP3_DRIVE, &group3_drive);
	eeprom_get_value(ftdi, GROUP3_SCHMITT, &group3_schmitt);
	eeprom_get_value(ftdi, GROUP3_SLEW, &group3_slew);
	eeprom_get_value(ftdi, CHIP_SIZE, &eeprom_size);
	eeprom_get_value(ftdi, CHANNEL_C_DRIVER, &channel_c_driver);
	eeprom_get_value(ftdi, CHANNEL_D_DRIVER, &channel_d_driver);
	eeprom_get_value(ftdi, CHIP_TYPE, &eeprom_type);
	eeprom_get_value(ftdi, CHANNEL_A_RS485, &channel_a_rs485);
	eeprom_get_value(ftdi, CHANNEL_B_RS485, &channel_b_rs485);
	eeprom_get_value(ftdi, CHANNEL_C_RS485, &channel_c_rs485);
	eeprom_get_value(ftdi, CHANNEL_D_RS485, &channel_d_rs485);

	eeprom_get_value(ftdi, POWER_SAVE, &power_save);
	eeprom_get_value(ftdi, CLOCK_POLARITY, &clock_polarity);
	eeprom_get_value(ftdi, DATA_ORDER, &data_order);
	eeprom_get_value(ftdi, FLOW_CONTROL, &flow_control);
	eeprom_get_value(ftdi, RELEASE_NUMBER, &release_number);
	eeprom_get_value(ftdi, EXTERNAL_OSCILLATOR, &external_oscillator);
	//eeprom_get_value(ftdi, USER_DATA_ADDR, &user_data_addr);
	
		
	
	printf("vendor_id=0x%04x\n", vid);
	printf("product_id=0x%04x\n", pid);
        printf("self_powered=%s\n", self_powered?"true":"false");
        printf("remote_wakeup=%s\n", remote_wakeup?"true":"false");
	printf("max_power=%d\n", max_power);
	printf("in_is_isochronous=%s\n", in_is_isochronous?"true":"false");
	printf("out_is_isochronous=%s\n", out_is_isochronous?"true":"false");
	printf("suspend_pull_downs=%s\n", suspend_pull_downs?"true":"false");
	printf("use_serial=%s\n", use_serial?"true":"false");
	printf("change_usb_version=%s\n", use_usb_version?"true":"false");
	printf("usb_version=%d\n", usb_version);
	eeprom_get_string(ftdi, manufacturer, product);
	printf("manufacturer=\"%s\"\n", manufacturer);
	printf("product=\"%s\"\n", product);
/*	fprintf(fp, "serial=\"%s\"\n", eeprom->serial);
*/ 	
	printf("high_current=%s\n", high_current?"true":"false");
	printf("eeprom_type=0x%04x\n", eeprom_type);
	printf("eeprom_size=0x%04x\n", eeprom_size);
	

	printf("invert_rxd=%s\n", (invert & INVERT_RXD)?"true":"false");
	printf("invert_txd=%s\n", (invert & INVERT_TXD)?"true":"false");
	printf("invert_rts=%s\n", (invert & INVERT_RTS)?"true":"false");
	printf("invert_cts=%s\n", (invert & INVERT_CTS)?"true":"false");
	printf("invert_dtr=%s\n", (invert & INVERT_DTR)?"true":"false");
	printf("invert_dsr=%s\n", (invert & INVERT_DSR)?"true":"false");
	printf("invert_dcd=%s\n", (invert & INVERT_DCD)?"true":"false");
	printf("invert_ri=%s\n", (invert & INVERT_RI)?"true":"false");

	
	printf("cha_type=%s\n", (channel_a_type == CHANNEL_IS_UART) ? "UART":
                                "FIFO");
	printf("chb_type=%s\n", (channel_b_type == CHANNEL_IS_UART) ? "UART":
                                "FIFO");	

	printf("cha_vcp=%s\n", (channel_a_driver == DRIVER_VCP) ? "true":"false");
	printf("chb_vcp=%s\n", (channel_b_driver == DRIVER_VCP) ? "true":"false");
	printf("chc_vcp=%s\n", (channel_c_driver == DRIVER_VCP) ? "true":"false");
	printf("chd_vcp=%s\n", (channel_d_driver == DRIVER_VCP) ? "true":"false");

        printf("cha_rs485=%s\n", channel_a_rs485 ? "true":"false");
	printf("chb_rs485=%s\n", channel_b_rs485 ? "true":"false");
	printf("chc_rs485=%s\n", channel_c_rs485 ? "true":"false");
	printf("chd_rs485=%s\n", channel_d_rs485 ? "true":"false");


	//printf("user_data_addr=0x%04x\n", user_data_addr);

	printf("\n");
	printf("is_not_pnp=0x%04x\n", is_not_pnp);
	printf("suspend_dbus7=0x%04x\n", suspend_dbus7);
	printf("high_current_a=%s\n", high_current_a?"true":"false");
	printf("high_current_b=%s\n", high_current_b?"true":"false");
	printf("group0_drive=0x%04x\n", group0_drive);
	printf("group0_schmitt=0x%04x\n", group0_schmitt);
	printf("group0_slew=0x%04x\n", group0_schmitt);
	printf("group1_drive=0x%04x\n", group1_drive);
	printf("group1_schmitt=0x%04x\n", group1_schmitt);
	printf("group1_slew=0x%04x\n", group1_schmitt);
	printf("group2_drive=0x%04x\n", group2_drive);
	printf("group2_schmitt=0x%04x\n", group2_schmitt);
	printf("group2_slew=0x%04x\n", group2_schmitt);
	printf("group3_drive=0x%04x\n", group3_drive);
	printf("group3_schmitt=0x%04x\n", group3_schmitt);
	printf("group3_slew=0x%04x\n", group3_schmitt);
	printf("cbus0=0x%04x\n",cbus0);
	printf("cbus1=0x%04x\n",cbus1);
	printf("cbus2=0x%04x\n",cbus2);
	printf("cbus3=0x%04x\n",cbus3);
	printf("cbus4=0x%04x\n",cbus4);
	printf("cbus5=0x%04x\n",cbush5);
	printf("cbus6=0x%04x\n",cbush6);
	printf("cbus7=0x%04x\n",cbush7);
	printf("cbus8=0x%04x\n",cbush8);
	printf("cbus9=0x%04x\n",cbush9);
	printf("invert=0x%04x\n", invert);
        printf("chip_type=%d\n", ftdi->type); // chip_type = "2232H"
	
	printf("power_save=0x%04x\n", power_save);
	printf("clock_polarity=0x%04x\n", clock_polarity);
	printf("data_order=0x%04x\n", data_order);
	printf("flow_control=0x%04x\n", flow_control);
	printf("release_number=0x%04x\n", release_number);
	printf("external_oscillator=0x%04x\n", external_oscillator);
	
        goto cleanup;
    }

    eeprom_set_value(ftdi, VENDOR_ID, cfg_getint(cfg, "vendor_id"));
    eeprom_set_value(ftdi, PRODUCT_ID, cfg_getint(cfg, "product_id"));

    eeprom_set_value(ftdi, SELF_POWERED, cfg_getbool(cfg, "self_powered"));
    eeprom_set_value(ftdi, REMOTE_WAKEUP, cfg_getbool(cfg, "remote_wakeup"));
    eeprom_set_value(ftdi, MAX_POWER, cfg_getint(cfg, "max_power"));

    eeprom_set_value(ftdi, IN_IS_ISOCHRONOUS, cfg_getbool(cfg, "in_is_isochronous"));
    eeprom_set_value(ftdi, OUT_IS_ISOCHRONOUS, cfg_getbool(cfg, "out_is_isochronous"));
    eeprom_set_value(ftdi, SUSPEND_PULL_DOWNS, cfg_getbool(cfg, "suspend_pull_downs"));

    eeprom_set_value(ftdi, USE_SERIAL, cfg_getbool(cfg, "use_serial"));
    eeprom_set_value(ftdi, USE_USB_VERSION, cfg_getbool(cfg, "change_usb_version"));
    eeprom_set_value(ftdi, USB_VERSION, cfg_getint(cfg, "usb_version"));
    eeprom_set_value(ftdi, CHIP_TYPE, cfg_getint(cfg, "eeprom_type"));

    eeprom_set_value(ftdi, HIGH_CURRENT, cfg_getbool(cfg, "high_current"));

    if (ftdi->type == TYPE_R)
    {
        if (cfg_getint(cfg, "cbus0") != -1)
            eeprom_set_value(ftdi, CBUS_FUNCTION_0, cfg_getint(cfg, "cbus0"));
        if (cfg_getint(cfg, "cbus1") != -1)
            eeprom_set_value(ftdi, CBUS_FUNCTION_1, cfg_getint(cfg, "cbus1"));
        if (cfg_getint(cfg, "cbus2") != -1)
            eeprom_set_value(ftdi, CBUS_FUNCTION_2, cfg_getint(cfg, "cbus2"));
        if (cfg_getint(cfg, "cbus3") != -1)
            eeprom_set_value(ftdi, CBUS_FUNCTION_3, cfg_getint(cfg, "cbus3"));
        if (cfg_getint(cfg, "cbus4") != -1)
            eeprom_set_value(ftdi, CBUS_FUNCTION_4, cfg_getint(cfg, "cbus4"));
    }
    else if (ftdi->type == TYPE_232H)
    {
        if (cfg_getint(cfg, "cbush0") != -1)
            eeprom_set_value(ftdi, CBUS_FUNCTION_0, cfg_getint(cfg, "cbush0"));
        if (cfg_getint(cfg, "cbush1") != -1)
            eeprom_set_value(ftdi, CBUS_FUNCTION_1, cfg_getint(cfg, "cbush1"));
        if (cfg_getint(cfg, "cbush2") != -1)
            eeprom_set_value(ftdi, CBUS_FUNCTION_2, cfg_getint(cfg, "cbush2"));
        if (cfg_getint(cfg, "cbush3") != -1)
            eeprom_set_value(ftdi, CBUS_FUNCTION_3, cfg_getint(cfg, "cbush3"));
        if (cfg_getint(cfg, "cbush4") != -1)
            eeprom_set_value(ftdi, CBUS_FUNCTION_4, cfg_getint(cfg, "cbush4"));
        if (cfg_getint(cfg, "cbush5") != -1)
            eeprom_set_value(ftdi, CBUS_FUNCTION_5, cfg_getint(cfg, "cbush5"));
        if (cfg_getint(cfg, "cbush6") != -1)
            eeprom_set_value(ftdi, CBUS_FUNCTION_6, cfg_getint(cfg, "cbush6"));
        if (cfg_getint(cfg, "cbush7") != -1)
            eeprom_set_value(ftdi, CBUS_FUNCTION_7, cfg_getint(cfg, "cbush7"));
        if (cfg_getint(cfg, "cbush8") != -1)
            eeprom_set_value(ftdi, CBUS_FUNCTION_8, cfg_getint(cfg, "cbush8"));
        if (cfg_getint(cfg, "cbush9") != -1)
            eeprom_set_value(ftdi, CBUS_FUNCTION_9, cfg_getint(cfg, "cbush9"));
    }
    else if (ftdi->type == TYPE_230X)
    {
        if (cfg_getint(cfg, "cbusx0") != -1)
            eeprom_set_value(ftdi, CBUS_FUNCTION_0, cfg_getint(cfg, "cbusx0"));
        if (cfg_getint(cfg, "cbusx1") != -1)
            eeprom_set_value(ftdi, CBUS_FUNCTION_1, cfg_getint(cfg, "cbusx1"));
        if (cfg_getint(cfg, "cbusx2") != -1)
            eeprom_set_value(ftdi, CBUS_FUNCTION_2, cfg_getint(cfg, "cbusx2"));
        if (cfg_getint(cfg, "cbusx3") != -1)
            eeprom_set_value(ftdi, CBUS_FUNCTION_3, cfg_getint(cfg, "cbusx3"));
    }

    int invert = 0;
    if (cfg_getbool(cfg, "invert_rxd")) invert |= INVERT_RXD;
    if (cfg_getbool(cfg, "invert_txd")) invert |= INVERT_TXD;
    if (cfg_getbool(cfg, "invert_rts")) invert |= INVERT_RTS;
    if (cfg_getbool(cfg, "invert_cts")) invert |= INVERT_CTS;
    if (cfg_getbool(cfg, "invert_dtr")) invert |= INVERT_DTR;
    if (cfg_getbool(cfg, "invert_dsr")) invert |= INVERT_DSR;
    if (cfg_getbool(cfg, "invert_dcd")) invert |= INVERT_DCD;
    if (cfg_getbool(cfg, "invert_ri")) invert |= INVERT_RI;
    eeprom_set_value(ftdi, INVERT, invert);

    if (cfg_getint(cfg, "cha_type") != -1)
        eeprom_set_value(ftdi, CHANNEL_A_TYPE, cfg_getint(cfg, "cha_type"));
    if (cfg_getint(cfg, "chb_type") != -1)
        eeprom_set_value(ftdi, CHANNEL_B_TYPE, cfg_getint(cfg, "chb_type"));

    eeprom_set_value(ftdi, CHANNEL_A_DRIVER,
                     cfg_getbool(cfg, "cha_vcp") ? DRIVER_VCP : 0);
    eeprom_set_value(ftdi, CHANNEL_B_DRIVER,
                     cfg_getbool(cfg, "chb_vcp") ? DRIVER_VCP : 0);
    eeprom_set_value(ftdi, CHANNEL_C_DRIVER,
                     cfg_getbool(cfg, "chc_vcp") ? DRIVER_VCP : 0);
    eeprom_set_value(ftdi, CHANNEL_D_DRIVER,
                     cfg_getbool(cfg, "chd_vcp") ? DRIVER_VCP : 0);

    eeprom_set_value(ftdi, CHANNEL_A_RS485, cfg_getbool(cfg, "cha_rs485"));
    eeprom_set_value(ftdi, CHANNEL_B_RS485, cfg_getbool(cfg, "chb_rs485"));
    eeprom_set_value(ftdi, CHANNEL_C_RS485, cfg_getbool(cfg, "chc_rs485"));
    eeprom_set_value(ftdi, CHANNEL_D_RS485, cfg_getbool(cfg, "chd_rs485"));

    /* Arbitrary user data */
    eeprom_set_value(ftdi, USER_DATA_ADDR, cfg_getint(cfg, "user_data_addr"));
    user_data_file = cfg_getstr(cfg, "user_data_file");
    if (user_data_file && strlen(user_data_file) > 0)
    {
        int data_size;
        struct stat st;

        printf("User data file: %s\n", user_data_file);
        /* Allocate a buffer for the user data */
        user_data_buffer = (char *)malloc(max_eeprom_size);
        if (user_data_buffer == NULL)
        {
            fprintf(stderr, "Malloc failed, aborting\n");
            goto cleanup;
        }

        if (stat(user_data_file, &st))
        {
            printf ("Can't stat user data file %s.\n", user_data_file);
            exit (-1);
        }
        if (st.st_size > max_eeprom_size)
            printf("Warning: %s is too big, only reading %d bytes\n",
                   user_data_file, max_eeprom_size);
        /* Read the user data file, no more than max_eeprom_size bytes */
        FILE *fp = fopen(user_data_file, "rb");
        if (fp == NULL)
        {
            printf ("Can't open user data file %s.\n", user_data_file);
            exit (-1);
        }
        data_size = fread(user_data_buffer, 1, max_eeprom_size, fp);
        fclose(fp);
        if (data_size < 1)
        {
            printf ("Can't read user data file %s.\n", user_data_file);
            exit (-1);
        }
        printf("User data size: %d\n", data_size);

        ftdi_set_eeprom_user_data(ftdi, user_data_buffer, data_size);
    }


    if (command == COMMAND_ERASE)
    {
        printf("FTDI erase eeprom: %d\n", ftdi_erase_eeprom(ftdi));
    }

    size_check = ftdi_eeprom_build(ftdi);
    eeprom_get_value(ftdi, CHIP_SIZE, &my_eeprom_size);

    if (size_check == -1)
    {
        printf ("Sorry, the eeprom can only contain %d bytes.\n", my_eeprom_size);
        goto cleanup;
    }
    else if (size_check < 0)
    {
        printf ("ftdi_eeprom_build(): error: %d\n", size_check);
        goto cleanup;
    }
    else
    {
        printf ("Used eeprom space: %d bytes\n", my_eeprom_size-size_check);
    }

    if (command == COMMAND_FLASH)
    {
        if (cfg_getbool(cfg, "flash_raw"))
        {
            if (filename != NULL && strlen(filename) > 0)
            {
                eeprom_buf = malloc(max_eeprom_size);
                FILE *fp = fopen(filename, "rb");
                if (fp == NULL)
                {
                    printf ("Can't open eeprom file %s.\n", filename);
                    exit (-1);
                }
                my_eeprom_size = fread(eeprom_buf, 1, max_eeprom_size, fp);
                fclose(fp);
                if (my_eeprom_size < 128)
                {
                    printf ("Can't read eeprom file %s.\n", filename);
                    exit (-1);
                }

                printf("Flashing raw eeprom from file %s (%d bytes)\n",
                       filename, my_eeprom_size);

                ftdi_set_eeprom_buf(ftdi, eeprom_buf, my_eeprom_size);
            } else
            {
                printf ("ERROR: flash_raw mode enabled, but no eeprom filename "
                        "given in config file.\n");
                exit (-1);
            }
        }
        printf ("FTDI write eeprom: %d\n", ftdi_write_eeprom(ftdi));
        libusb_reset_device(ftdi->usb_dev);
    }

    // Write to file?
    if (filename != NULL && strlen(filename) > 0 && !cfg_getbool(cfg, "flash_raw"))
    {
        fp = fopen(filename, "w");
        if (fp == NULL)
        {
            printf ("Can't write eeprom file.\n");
            exit (-1);
        }
        else
            printf ("Writing to file: %s\n", filename);

        if (eeprom_buf == NULL)
            eeprom_buf = malloc(my_eeprom_size);
        ftdi_get_eeprom_buf(ftdi, eeprom_buf, my_eeprom_size);

        fwrite(eeprom_buf, my_eeprom_size, 1, fp);
        fclose(fp);
    }

cleanup:
    if (eeprom_buf)
        free(eeprom_buf);
    if (user_data_buffer)
        free(user_data_buffer);
    if (command > 0)
    {
        printf("FTDI close: %d\n", ftdi_usb_close(ftdi));
    }

    ftdi_deinit (ftdi);
    ftdi_free (ftdi);

    cfg_free(cfg);

    printf("\n");
    return 0;
}
