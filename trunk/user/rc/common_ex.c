/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <syslog.h>
#include <stdarg.h>
#include <arpa/inet.h>

#include <shutils.h>
#include <ralink.h>
#include <flash_mtd.h>
#include <nvram/bcmnvram.h>

#include "rc.h"


#define XSTR(s) STR(s)
#define STR(s) #s

long uptime(void)
{
	struct sysinfo info;
	sysinfo(&info);

	return info.uptime;
}

int rand_seed_by_time(void)
{
	time_t atime;

	/* make a random number and set the top and bottom bits */
	time(&atime);
	srand((unsigned long)atime);

	return rand();
}


// oleg patch ~
in_addr_t
inet_addr_(const char *cp)
{
       struct in_addr a;

       if (!inet_aton(cp, &a))
	       return INADDR_ANY;
       else
	       return a.s_addr;
}
// ~ oleg patch

/* remove space in the end of string */
char *trim_r(char *str)
{
	int i;

	i=strlen(str);

	while (i>=1)
	{
		if (*(str+i-1) == ' ' || *(str+i-1) == 0x0a || *(str+i-1) == 0x0d) *(str+i-1)=0x0;
		else break;
		i--;
	}
	return (str);
}

/* convert mac address format from XXXXXXXXXXXX to XX:XX:XX:XX:XX:XX */
char *mac_conv(char *mac_name, int idx, char *buf)
{
	char *mac, name[32];
	int i, j;

	if (idx!=-1)
		sprintf(name, "%s%d", mac_name, idx);
	else sprintf(name, "%s", mac_name);

	mac = nvram_safe_get(name);

	if (strlen(mac)==0) 
	{
		buf[0] = 0;
	}
	else
	{
		j=0;	
		for (i=0; i<12; i++)
		{		
			if (i!=0&&i%2==0) buf[j++] = ':';
			buf[j++] = mac[i];
		}
		buf[j] = 0;	// oleg patch
	}

	return (buf);
}

// 2010.09 James. {
/* convert mac address format from XX:XX:XX:XX:XX:XX to XXXXXXXXXXXX */
char *mac_conv2(char *mac_name, int idx, char *buf)
{
	char *mac, name[32];
	int i, j;

	if(idx != -1)	
		sprintf(name, "%s%d", mac_name, idx);
	else
		sprintf(name, "%s", mac_name);

	mac = nvram_safe_get(name);

	if(strlen(mac) == 0 || strlen(mac) != 17)
		buf[0] = 0;
	else{
		for(i = 0, j = 0; i < 17; ++i){
			if(i%3 != 2){
				buf[j] = mac[i];
				++j;
			}

			buf[j] = 0;
		}
	}

	return(buf);
}
// 2010.09 James. }

int valid_subver(char subfs)
{
	printf("validate subfs: %c\n", subfs);	// tmp test
	if(((subfs >= 'a') && (subfs <= 'z' )) || ((subfs >= 'A') && (subfs <= 'Z' )))
		return 1;
	else
		return 0;
}

void getsyspara(void)
{
	unsigned char buffer[32];
	int i;
	char macaddr[]="00:11:22:33:44:55";
	char macaddr2[]="00:11:22:33:44:56";
	char macaddr3[]="001122334457";
	char macaddr4[]="001122334458";
	char ea[ETHER_ADDR_LEN];
	char country_code[3];
	char pin[9];
	char productid[13];
	char fwver[8], fwver_sub[16];
	char blver[20];
	unsigned char txbf_para[33];
	
	/* /dev/mtd/2, RF parameters, starts from 0x40000 */
	memset(buffer, 0, sizeof(buffer));
	memset(country_code, 0, sizeof(country_code));
	memset(pin, 0, sizeof(pin));
	memset(productid, 0, sizeof(productid));
	memset(fwver, 0, sizeof(fwver));
	memset(fwver_sub, 0, sizeof(fwver_sub));
	memset(txbf_para, 0, sizeof(txbf_para));

	if (FRead(buffer, OFFSET_MAC_ADDR, 6)<0)
	{
		dbg("READ MAC address: Out of scope\n");
	}
	else
	{
		if (buffer[0]!=0xff)
		{
			ether_etoa(buffer, macaddr);
			ether_etoa2(buffer, macaddr3);
		}
	}
	
	if (FRead(buffer, OFFSET_MAC_ADDR_2G, 6)<0)
	{
		dbg("READ MAC address 2G: Out of scope\n");
	}
	else
	{
		if (buffer[0]!=0xff)
		{
			ether_etoa(buffer, macaddr2);
			ether_etoa2(buffer, macaddr4);
		}
	}

	nvram_set("il0macaddr", macaddr);
	nvram_set("il1macaddr", macaddr2);
	nvram_set("et0macaddr", macaddr);
	nvram_set("br0hexaddr", macaddr3);
	nvram_set("wanhexaddr", macaddr4);
	
	if (FRead(buffer, OFFSET_MAC_GMAC0, 6)<0)
	{
		dbg("READ MAC address GMAC0: Out of scope\n");
	}
	else
	{
		if (buffer[0]==0xff)
		{
			if (ether_atoe(macaddr, ea))
				FWrite(ea, OFFSET_MAC_GMAC0, 6);
		}
	}
	
	if (FRead(buffer, OFFSET_MAC_GMAC2, 6)<0)
	{
		dbg("READ MAC address GMAC2: Out of scope\n");
	}
	else
	{
		if (buffer[0]==0xff)
		{
			if (ether_atoe(macaddr2, ea))
				FWrite(ea, OFFSET_MAC_GMAC2, 6);
		}
	}
	
	/* reserved for Ralink. used as ASUS country code. */
	if (FRead(country_code, OFFSET_COUNTRY_CODE, 2)<0)
	{
		dbg("READ ASUS country code: Out of scope\n");
		strcpy(country_code, "GB");
	}
	else
	{
		country_code[2] = 0;
		if ((unsigned char)country_code[0]==0xff)
			strcpy(country_code, "GB");
	}
	
	if (strlen(nvram_safe_get("rt_country_code")) == 0)
	{
		nvram_set("rt_country_code", country_code);
	}
	
	if (strlen(nvram_safe_get("wl_country_code")) == 0)
	{
		nvram_set("wl_country_code", country_code);
	}
	
	if (!strcasecmp(nvram_safe_get("wl_country_code"), "BR"))
		nvram_set("wl_country_code", "UZ");
	
	/* reserved for Ralink. used as ASUS pin code. */
	if (FRead(pin, OFFSET_PIN_CODE, 8)<0)
	{
		dbg("READ ASUS pin code: Out of scope\n");
		nvram_set("wl_pin_code", "");
	}
	else
	{
		if ((unsigned char)pin[0]!=0xff)
			nvram_set("secret_code", pin);
		else
			nvram_set("secret_code", "12345670");
	}

#if defined(USE_RT3352_MII)
 #define EEPROM_INIC_SIZE (512)
 #define EEPROM_INIT_ADDR 0x48000
	{
		char eeprom[EEPROM_INIC_SIZE];
		if(FRead(eeprom, EEPROM_INIT_ADDR, sizeof(eeprom)) < 0)
		{
			dbg("READ iNIC EEPROM: Out of scope!\n");
		}
		else
		{
			FILE *fp;
			if((fp = fopen("/etc/Wireless/iNIC/iNIC_e2p.bin", "w")))
			{
				fwrite(eeprom, sizeof(eeprom), 1, fp);
				fclose(fp);
			}
		}
	}
#endif

	/* /dev/mtd/3, firmware, starts from 0x50000 */
	if (FRead(buffer, 0x50020, sizeof(buffer))<0)
	{
		dbg("READ firmware header: Out of scope\n");
		nvram_set("productid", "unknown");
		nvram_set("firmver", "unknown");
	}
	else
	{
		strncpy(productid, buffer + 4, 12);
		productid[12] = 0;
		
		if(valid_subver(buffer[27]))
			sprintf(fwver_sub, "%d.%d.%d.%d%c", buffer[0], buffer[1], buffer[2], buffer[3], buffer[27]);
		else
			sprintf(fwver_sub, "%d.%d.%d.%d", buffer[0], buffer[1], buffer[2], buffer[3]);
		
#if defined(FWBLDSTR)
		sprintf(fwver_sub, "%s-%s", fwver_sub, FWBLDSTR);
#endif
		sprintf(fwver, "%d.%d.%d.%d", buffer[0], buffer[1], buffer[2], buffer[3]);
		nvram_set("productid", trim_r(productid));
		nvram_set("firmver", trim_r(fwver));
		nvram_set("firmver_sub", trim_r(fwver_sub));
	}

	memset(buffer, 0, sizeof(buffer));
	FRead(buffer, OFFSET_BOOT_VER, 4);
	sprintf(blver, "%s-0%c-0%c-0%c-0%c", trim_r(productid), buffer[0], buffer[1], buffer[2], buffer[3]);
	nvram_set("blver", trim_r(blver));

	int count_0xff = 0;
	if (FRead(txbf_para, OFFSET_TXBF_PARA, 33) < 0)
	{
		dbg("READ TXBF PARA address: Out of scope\n");
	}
	else
	{
		for (i = 0; i < 33; i++)
		{
			if (txbf_para[i] == 0xff)
				count_0xff++;
		}
	}

	if (count_0xff == 33)
		nvram_set("wl_txbf_en", "0");
	else
		nvram_set("wl_txbf_en", "1");
}

void wan_netmask_check(void)
{
	unsigned int ip, gw, nm, lip, lnm;

	if (nvram_match("wan0_proto", "static") ||
	    nvram_match("wan0_proto", "pptp") || 
	    nvram_match("wan0_proto", "l2tp"))
	{
		ip = inet_addr(nvram_safe_get("wan0_ipaddr"));
		gw = inet_addr(nvram_safe_get("wan0_gateway"));
		nm = inet_addr(nvram_safe_get("wan0_netmask"));
		
		lip = inet_addr(nvram_safe_get("lan_ipaddr"));
		lnm = inet_addr(nvram_safe_get("lan_netmask"));
		
		if (ip==0x0 && (nvram_invmatch("wan0_proto", "static")))
			return;
		
		if (ip==0x0 || ip==0xffffffff || (ip&lnm)==(lip&lnm))
		{
			nvram_set("wan0_ipaddr", "1.1.1.1");
			nvram_set("wan0_netmask", "255.0.0.0");	
		}
		
		// check netmask here
		if (gw!=0 && gw!=0xffffffff && (ip&nm)!=(gw&nm))
		{
			for (nm=0xffffffff;nm!=0;nm=(nm>>8))
			{
				if ((ip&nm)==(gw&nm)) break;
			}
			
			if (nm==0xffffffff) nvram_set("wan0_netmask", "255.255.255.255");
			else if (nm==0xffffff) nvram_set("wan0_netmask", "255.255.255.0");
			else if (nm==0xffff) nvram_set("wan0_netmask", "255.255.0.0");
			else if (nm==0xff) nvram_set("wan0_netmask", "255.0.0.0");
			else nvram_set("wan0_netmask", "0.0.0.0");
		}
		
		nvram_set("wanx_ipaddr", nvram_safe_get("wan0_ipaddr"));	// oleg patch, he suggests to mark the following 3 lines
		nvram_set("wanx_netmask", nvram_safe_get("wan0_netmask"));
		nvram_set("wanx_gateway", nvram_safe_get("wan0_gateway"));
	}
}

void init_router_mode(void)
{
	if (!nvram_get("sw_mode"))
		nvram_set("sw_mode", "1");

	if (nvram_match("sw_mode", "1"))		// Gateway mode
	{
		nvram_set("wan_nat_x", "1");
		nvram_set("wan_route_x", "IP_Routed");
	}
	else if (nvram_match("sw_mode", "4"))		// Router mode
	{
		nvram_set("wan_nat_x", "0");
		nvram_set("wan_route_x", "IP_Routed");
	}
	else if (nvram_match("sw_mode", "3"))		// AP mode
	{
		nvram_set("wan_nat_x", "0");
		nvram_set("wan_route_x", "IP_Bridged");
	}
	else
	{
		nvram_set("sw_mode", "1");
		nvram_set("wan_nat_x", "1");
		nvram_set("wan_route_x", "IP_Routed");
	}
}

void update_router_mode(void)
{
	if (nvram_match("wan_route_x", "IP_Routed"))
	{
		if (!nvram_get_int("wan_nat_x"))
			nvram_set("sw_mode", "4");	// Gateway mode
		else
			nvram_set("sw_mode", "1");	// Router mode
	}
}

/* This function is used to map nvram value from asus to Broadcom */
void convert_asus_values(int skipflag)
{
	char ifnames[36];
	char nvram_name[32];
	int i, j;

	nvram_unset("rc_service");
	nvram_unset("manually_disconnect_wan");

	if (nvram_match("macfilter_enable_x", "disabled"))
		nvram_set("macfilter_enable_x", "0");
	
	if (nvram_match("wl_frameburst", "0"))
		nvram_set("wl_frameburst", "off");
	
	if (nvram_match("wl_preauth", "1"))
		nvram_set("wl_preauth", "enabled");
	else if (nvram_match("wl_preauth", "0"))
		nvram_set("wl_preauth", "disabled");

	if (nvram_match("wan_proto", "cdma"))
		nvram_set("wan_proto", "dhcp");

	/* Clean MFG test values when boot */
	nvram_set("asus_mfg", "0");

	if (nvram_match("wl_wpa_mode", ""))
		nvram_set("wl_wpa_mode", "0");

	/* Mac filter */

//2008.09 magic }

	if (!skipflag)
	{
		set_usb_modem_state(0);
		/* Direct copy value */
		/* LAN Section */
		reset_lan_vars();
		
		// WAN section
		reset_wan_vars(1);
	}

	if (!skipflag)
	{
		if (is_ap_mode())
		{
			sprintf(ifnames, "%s", IFNAME_BR);
			nvram_set("lan_ifnames_t", ifnames);
			nvram_set("br0_ifnames", ifnames);	// 2008.09 magic
			nvram_set("router_disable", "1");
		}
		else
		{
			memset(ifnames, 0, sizeof(ifnames));
			strcpy(ifnames, nvram_safe_get("lan_ifnames"));
			nvram_set("lan_ifnames_t", ifnames);
			nvram_set("br0_ifnames", ifnames);
			nvram_set("router_disable", "0");
		}
	}

	// clean some temp variables
	if (!skipflag)
	{
	nvram_set("usb_dev_state", "none");

	nvram_set("usb_path", "");
	nvram_set("usb_path1", "");
	nvram_set("usb_path2", "");
	for (i = 1; i < 3 ; i++)
	{
		for (j = 0; j < 16 ; j++)
		{
			sprintf(nvram_name, "usb_path%d_fs_path%d", i, j);
			nvram_unset(nvram_name);
		}
	}
	nvram_set("usb_path1_index", "0");
	nvram_set("usb_path1_add", "0");
	nvram_set("usb_path1_vid", "");
	nvram_set("usb_path1_pid", "");
	nvram_set("usb_path1_manufacturer", "");
	nvram_set("usb_path1_product", "");
	nvram_set("usb_path1_removed", "0");
	nvram_set("usb_path2_index", "0");
	nvram_set("usb_path2_add", "0");
	nvram_set("usb_path2_vid", "");
	nvram_set("usb_path2_pid", "");
	nvram_set("usb_path2_manufacturer", "");
	nvram_set("usb_path2_product", "");
	nvram_set("usb_path2_removed", "0");
	nvram_set("usb_path1_act", "");
	nvram_set("usb_path2_act", "");
	
	}

	time_zone_x_mapping();

	if (!skipflag)
	{
	nvram_unset("support_cdma");

	nvram_set("reboot", "");

	nvram_set("networkmap_fullscan", "0");	// 2008.07 James.
	nvram_set("link_internet", "2");
	nvram_set("detect_timestamp", "0");	// 2010.10 James.
	nvram_set("fullscan_timestamp", "0");
	nvram_set("renew_timestamp", "0");
	nvram_unset("wan_gateway_tmp");
	nvram_unset("wan_ipaddr_tmp");
	nvram_unset("wan_netmask_tmp");

	nvram_set("done_auto_mac", "0");	// 2010.09 James.

	nvram_set("reload_svc_wl", "0");
	nvram_set("reload_svc_rt", "0");

	nvram_unset("reboot_timestamp");

	nvram_unset("ddns_cache");
	nvram_unset("ddns_ipaddr");
	nvram_unset("ddns_status");
	nvram_unset("ddns_updated");
	}
}

void restart_all_sysctl(void)
{
	set_ppp_limit_cpu();
	set_pppoe_passthrough();
}

void logmessage(char *logheader, char *fmt, ...)
{
  va_list args;
  char buf[512];

  va_start(args, fmt);

  vsnprintf(buf, sizeof(buf), fmt, args);
  openlog(logheader, 0, 0);
  syslog(0, buf);
  closelog();
  va_end(args);
}

void char_to_ascii(char *output, char *input)/* Transfer Char to ASCII */
{						   /* Cherry_Cho added in 2006/9/29. */
	int i;
	char tmp[10];
	char *ptr;

	ptr = output;

	for ( i=0; i<strlen(input); i++ )
	{
		if ((input[i]>='0' && input[i] <='9')
		   ||(input[i]>='A' && input[i]<='Z')
		   ||(input[i] >='a' && input[i]<='z')
		   || input[i] == '!' || input[i] == '*'
		   || input[i] == '(' || input[i] == ')'
		   || input[i] == '_' || input[i] == '-'
		   || input[i] == '\'' || input[i] == '.')
		{
			*ptr = input[i];
			ptr ++;
		}
		else
		{
			sprintf(tmp, "%%%.02X", input[i]);
			strcpy(ptr, tmp);
			ptr += 3;
		}
	}
	*(ptr) = '\0';
}

int fput_string(const char *name, const char *value)
{
	FILE *fp;

	fp = fopen(name, "w");
	if (fp) {
		fputs(value, fp);
		fclose(fp);
		return 0;
	} else {
		perror(name);
		return errno;
	}
}

int fput_int(const char *name, int value)
{
	char svalue[32];
	sprintf(svalue, "%d", value);
	return fput_string(name, svalue);
}


