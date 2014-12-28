/*-
 * Copyright (c) 2007 Yahoo!, Inc.
 * All rights reserved.
 * Written by: John Baldwin <jhb@FreeBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static const char rcsid[] =
  "$FreeBSD$";
#endif /* not lint */

#include <sys/types.h>

#include <err.h>
#include <stdio.h>
#include <sys/agpio.h>
#include <sys/pciio.h>
#include <libxo/xo.h>

#include <dev/agp/agpreg.h>
#include <dev/pci/pcireg.h>

#include "pciconf.h"

static void	list_ecaps(int fd, struct pci_conf *p);

static void
cap_power(int fd, struct pci_conf *p, uint8_t ptr)
{
	uint16_t cap, status;

	xo_emit("{e:capability-name/Power Management}");
	cap = read_config(fd, &p->pc_sel, ptr + PCIR_POWER_CAP, 2);
	status = read_config(fd, &p->pc_sel, ptr + PCIR_POWER_STATUS, 2);
	//printf("powerspec %d  supports D0%s%s D3  current D%d",
	xo_emit("powerspec {:power-management-version/%d}  ",
	    cap & PCIM_PCAP_SPEC);
	xo_emit("supports {:supported-power-states/D0%s%s D3}  ",
	    cap & PCIM_PCAP_D1SUPP ? " D1" : "",
	    cap & PCIM_PCAP_D2SUPP ? " D2" : "");
	xo_emit("current {q:current-power-state/D%d}",
	    status & PCIM_PSTAT_DMASK);
}

static void
cap_agp(int fd, struct pci_conf *p, uint8_t ptr)
{
	uint32_t status, command;

	status = read_config(fd, &p->pc_sel, ptr + AGP_STATUS, 4);
	command = read_config(fd, &p->pc_sel, ptr + AGP_CAPID, 4);
	//printf("AGP ");
	xo_emit("{:capability-name/AGP} ");
	if (AGP_MODE_GET_MODE_3(status)) {
		//printf("v3 ");
		xo_emit("v{:agp-version/%d} ", 3);
		if (AGP_MODE_GET_RATE(status) & AGP_MODE_V3_RATE_8x)
			//printf("8x ");
			xo_emit("{:agp-speed/8}x ");
		if (AGP_MODE_GET_RATE(status) & AGP_MODE_V3_RATE_4x)
			//printf("4x ");
			xo_emit("{:agp-speed/4}x ");
	} else {
		//xo_emit("v{:agp-version/%d} ", 2);
		if (AGP_MODE_GET_RATE(status) & AGP_MODE_V2_RATE_4x)
			//printf("4x ");
			xo_emit("{:agp-speed/4}x ");
		if (AGP_MODE_GET_RATE(status) & AGP_MODE_V2_RATE_2x)
			//printf("2x ");
			xo_emit("{:agp-speed/2}x ");
		if (AGP_MODE_GET_RATE(status) & AGP_MODE_V2_RATE_1x)
			//printf("1x ");
			xo_emit("{:agp-speed/1}x ");
	}
	if (AGP_MODE_GET_SBA(status))
		//printf("SBA ");
		xo_emit("SBA ");
	if (AGP_MODE_GET_AGP(command)) {
		//printf("enabled at ");
		xo_emit("{:side-band-addressing/enabled} at ");
		if (AGP_MODE_GET_MODE_3(command)) {
			//printf("v3 ");
			xo_emit("v{:agp-version/%d} ", 3);
			switch (AGP_MODE_GET_RATE(command)) {
			case AGP_MODE_V3_RATE_8x:
				//printf("8x ");
				xo_emit("{:side-band-addressing-speed/8}x ");
				break;
			case AGP_MODE_V3_RATE_4x:
				//printf("4x ");
				xo_emit("{:side-band-addressing-speed/4}x ");
				break;
			}
		} else
			switch (AGP_MODE_GET_RATE(command)) {
			case AGP_MODE_V2_RATE_4x:
				//printf("4x ");
				xo_emit("{:side-band-addressing-speed/4}x ");
				break;
			case AGP_MODE_V2_RATE_2x:
				//printf("2x ");
				xo_emit("{:side-band-addressing-speed/2}x ");
				break;
			case AGP_MODE_V2_RATE_1x:
				//printf("1x ");
				xo_emit("{:side-band-addressing-speed/1}x ");
				break;
			}
		if (AGP_MODE_GET_SBA(command))
			//printf("SBA ");
			xo_emit("SBA ");
	} else
		//printf("disabled");
		xo_emit("{:side-band-addressing/disabled}");
}

static void
cap_vpd(int fd, struct pci_conf *p, uint8_t ptr)
{

	//printf("VPD");
	xo_emit("{:capability-name/VPD}");
	xo_emit("{q:vital-product-data/true}");
}

static void
cap_msi(int fd, struct pci_conf *p, uint8_t ptr)
{
	uint16_t ctrl;
	int msgnum;

	ctrl = read_config(fd, &p->pc_sel, ptr + PCIR_MSI_CTRL, 2);
	msgnum = 1 << ((ctrl & PCIM_MSICTRL_MMC_MASK) >> 1);
	//printf("MSI supports %d message%s%s%s ", msgnum,
	xo_emit("{:capability-name/MSI} ");
	xo_emit("supports {:multi-message-capable/%d} message{d:/%s%s%s} ", 
	    msgnum, (msgnum == 1) ? "" : "s",
	    (ctrl & PCIM_MSICTRL_64BIT) ? ", 64 bit" : "",
	    (ctrl & PCIM_MSICTRL_VECTOR) ? ", vector masks" : "");
	xo_emit("{e:capable-of-64-bits/%s}",
	    (ctrl & PCIM_MSICTRL_64BIT) ? "true" : "false");
	xo_emit("{e:per-vector-mask-capable/%s}",
	    (ctrl & PCIM_MSICTRL_VECTOR) ? "true" : "false");
	if (ctrl & PCIM_MSICTRL_MSI_ENABLE) {
		msgnum = 1 << ((ctrl & PCIM_MSICTRL_MME_MASK) >> 4);
		//printf("enabled with %d message%s", msgnum,
		xo_emit("enabled with {:multiple-message-enable/%d} message{d:/%s}", 
		    msgnum, (msgnum == 1) ? "" : "s");
	}
}

static void
cap_pcix(int fd, struct pci_conf *p, uint8_t ptr)
{
	uint32_t status;
	int comma, max_splits, max_burst_read;

	status = read_config(fd, &p->pc_sel, ptr + PCIXR_STATUS, 4);
	//printf("PCI-X ");
	xo_emit("{:capability-name/PCI-X} ");	// XXX not yet ready
	if (status & PCIXM_STATUS_64BIT)
		//printf("64-bit ");
		xo_emit("{:pcix-feature/64-bit} ");
	if ((p->pc_hdr & PCIM_HDRTYPE) == 1)
		//printf("bridge ");
		xo_emit("{:pcix-device-type/bridge} ");
	if ((p->pc_hdr & PCIM_HDRTYPE) != 1 || (status & (PCIXM_STATUS_133CAP |
	    PCIXM_STATUS_266CAP | PCIXM_STATUS_533CAP)) != 0)
		//printf("supports");
		xo_emit("supports");
	comma = 0;
	if (status & PCIXM_STATUS_133CAP) {
		//printf("%s 133MHz", comma ? "," : "");
		xo_emit("{:supported-bus-speed-133mhz/%s 133MHz/true}", 
		    comma ? "," : "");
		comma = 1;
	}
	if (status & PCIXM_STATUS_266CAP) {
		//printf("%s 266MHz", comma ? "," : "");
		xo_emit("{:supported-bus-speed-266mhz/%s 266MHz/true}", 
		    comma ? "," : "");
		comma = 1;
	}
	if (status & PCIXM_STATUS_533CAP) {
		//printf("%s 533MHz", comma ? "," : "");
		xo_emit("{:supported-bus-speed-533mhz/%s 533MHz/true}", 
		    comma ? "," : "");
		comma = 1;
	}
	if ((p->pc_hdr & PCIM_HDRTYPE) == 1)
		return;
	switch (status & PCIXM_STATUS_MAX_READ) {
	case PCIXM_STATUS_MAX_READ_512:
		max_burst_read = 512;
		break;
	case PCIXM_STATUS_MAX_READ_1024:
		max_burst_read = 1024;
		break;
	case PCIXM_STATUS_MAX_READ_2048:
		max_burst_read = 2048;
		break;
	case PCIXM_STATUS_MAX_READ_4096:
		max_burst_read = 4096;
		break;
	}
	switch (status & PCIXM_STATUS_MAX_SPLITS) {
	case PCIXM_STATUS_MAX_SPLITS_1:
		max_splits = 1;
		break;
	case PCIXM_STATUS_MAX_SPLITS_2:
		max_splits = 2;
		break;
	case PCIXM_STATUS_MAX_SPLITS_3:
		max_splits = 3;
		break;
	case PCIXM_STATUS_MAX_SPLITS_4:
		max_splits = 4;
		break;
	case PCIXM_STATUS_MAX_SPLITS_8:
		max_splits = 8;
		break;
	case PCIXM_STATUS_MAX_SPLITS_12:
		max_splits = 12;
		break;
	case PCIXM_STATUS_MAX_SPLITS_16:
		max_splits = 16;
		break;
	case PCIXM_STATUS_MAX_SPLITS_32:
		max_splits = 32;
		break;
	}
	//printf("%s %d burst read, %d split transaction%s", comma ? "," : "",
	xo_emit("{d:/%s }{:maximum-read-burst/%d} burst read, ",
	    comma ? "," : "", max_burst_read);
	xo_emit("{:maximum-split-transactions/%d} split transaction{d:/%s}", 
	    max_splits, max_splits == 1 ? "" : "s");
}

static void
cap_ht(int fd, struct pci_conf *p, uint8_t ptr)
{
	uint32_t reg;
	uint16_t command;

	command = read_config(fd, &p->pc_sel, ptr + PCIR_HT_COMMAND, 2);
	//printf("HT ");
	xo_emit("HT {eq:capability-name/Hyper-Transport}");
	if ((command & 0xe000) == PCIM_HTCAP_SLAVE)
		//printf("slave");
		xo_emit("slave{eq:hyper-transport-mode/slave}");
	else if ((command & 0xe000) == PCIM_HTCAP_HOST)
		//printf("host");
		xo_emit("host{eq:hyper-transport-mode/host}");
	else
		switch (command & PCIM_HTCMD_CAP_MASK) {
		case PCIM_HTCAP_SWITCH:
			//printf("switch");
			xo_emit("{q:hyper-transport-mode/switch}");
			break;
		case PCIM_HTCAP_INTERRUPT:
			//printf("interrupt");
			xo_emit("{q:hyper-transport-mode/interrupt}");
			break;
		case PCIM_HTCAP_REVISION_ID:
			//printf("revision ID");
			xo_emit("{q:hyper-transport-mode/revision ID}");
			break;
		case PCIM_HTCAP_UNITID_CLUMPING:
			//printf("unit ID clumping");
			xo_emit("{q:hyper-tranport-mode/unit ID clumping}");
			break;
		case PCIM_HTCAP_EXT_CONFIG_SPACE:
			//printf("extended config space");
			xo_emit("{q:hyper-transport-mode/extended config space}");
			break;
		case PCIM_HTCAP_ADDRESS_MAPPING:
			//printf("address mapping");
			xo_emit("{q:hyper-transport-mode/address mapping}");
			break;
		case PCIM_HTCAP_MSI_MAPPING:
			//printf("MSI %saddress window %s at 0x",
			xo_emit("{d:/MSI %saddress window %s at }",
			    command & PCIM_HTCMD_MSI_FIXED ? "fixed " : "",
			    command & PCIM_HTCMD_MSI_ENABLE ? "enabled" :
			    "disabled");
			if (command & PCIM_HTCMD_MSI_FIXED)
				//printf("fee00000");
				xo_emit("{:msi-window-address/0xfee00000}");
			else {
				reg = read_config(fd, &p->pc_sel,
				    ptr + PCIR_HTMSI_ADDRESS_HI, 4);
				//if (reg != 0)
					//printf("%08x", reg);
				if (reg != 0)
					xo_emit("{:msi-window-address/0x%08x%08x}", 
					    reg, 
					    read_config(fd, &p->pc_sel, 
						ptr + PCIR_HTMSI_ADDRESS_LO, 4));
				else
					xo_emit("{:msi-window-address/0x%08x}", 
					    read_config(fd, &p->pc_sel, 
						ptr + PCIR_HTMSI_ADDRESS_LO, 4));
			}
			break;
		case PCIM_HTCAP_DIRECT_ROUTE:	// XXX not yet ready
			//printf("direct route");
			xo_emit("{q:hyper-transport-mode/direct route}");
			break;
		case PCIM_HTCAP_VCSET:
			//printf("VC set");
			xo_emit("{q:hyper-transport-mode/VC set}");
			break;
		case PCIM_HTCAP_RETRY_MODE:
			//printf("retry mode");
			xo_emit("{q:hyper-transport-mode/retry mode}");
			break;
		case PCIM_HTCAP_X86_ENCODING:
			//printf("X86 encoding");
			xo_emit("{q:hyper-transport-mode/X86 encoding}");
			break;
		case PCIM_HTCAP_GEN3:
			//printf("Gen3");
			xo_emit("{q:hyper-transport-mode/Gen3}");
			break;
		case PCIM_HTCAP_FLE:
			//printf("function-level extension");
			xo_emit("{q:hyper-transport-mode/function-level extension}");
			break;
		case PCIM_HTCAP_PM:
			//printf("power management");
			xo_emit("{q:hyper-transport-mode/power management}");
			break;
		case PCIM_HTCAP_HIGH_NODE_COUNT:
			//printf("high node count");
			xo_emit("{q:hyper-transport-mode/high node count}");
			break;
		default:
			//printf("unknown %02x", command);
			xo_emit("{q:hyper-transport-mode/unknown %02x{:/}", 
			    command);
			break;
		}
}

static void
cap_vendor(int fd, struct pci_conf *p, uint8_t ptr)
{
	uint8_t length;

	length = read_config(fd, &p->pc_sel, ptr + PCIR_VENDOR_LENGTH, 1);
	//printf("vendor (length %d)", length);
	xo_emit("{eq:capability-name/Vendor Specific Information}");
	xo_emit("vendor (length {:vendor-specific-data-length/%d})", length);
	if (p->pc_vendor == 0x8086) {
		/* Intel */
		uint8_t version;

		version = read_config(fd, &p->pc_sel, ptr + PCIR_VENDOR_DATA,
		    1);
		//printf(" Intel cap %d version %d", version >> 4, version & 0xf);
		xo_emit(" {:vendor-name/Intel} cap {:intel-specific-capabilities/%d} "
		    "version {:version/%d}", 
		    version >> 4, version & 0xf);
		if (version >> 4 == 1 && length == 12) {
			/* Feature Detection */
			uint32_t fvec;
			int comma;

			comma = 0;
			fvec = read_config(fd, &p->pc_sel, ptr +
			    PCIR_VENDOR_DATA + 5, 4);
			//printf("\n\t\t features:");
			xo_emit("\n\t\t features:");
			if (fvec & (1 << 0)) {
				//printf(" AMT");
				xo_emit(" AMT{eq:feature-amt/true}");
				comma = 1;
			}
			fvec = read_config(fd, &p->pc_sel, ptr +
			    PCIR_VENDOR_DATA + 1, 4);
			if (fvec & (1 << 21)) {
				//printf("%s Quick Resume", comma ? "," : "");
				xo_emit("{d:/%s} Quick Resume{eq:feature-quick-resume/true}", 
				    comma ? "," : "");
				comma = 1;
			}
			if (fvec & (1 << 18)) {
				//printf("%s SATA RAID-5", comma ? "," : "");
				xo_emit("{d:/%s} SATA RAID-5{eq:feature-raid-5/true}", 
				    comma ? "," : "");
				comma = 1;
			}
			if (fvec & (1 << 9)) {
				//printf("%s Mobile", comma ? "," : "");
				xo_emit("{d:/%s} Mobile{eq:feature-mobile/true}", comma ? "," : "");
				comma = 1;
			}
			if (fvec & (1 << 7)) {
				//printf("%s 6 PCI-e x1 slots", comma ? "," : "");
				xo_emit("{d:/%s} 6 PCI-e x1 slots{e:pci-express-slots/6}", 
				    comma ? "," : "");
				comma = 1;
			} else {
				//printf("%s 4 PCI-e x1 slots", comma ? "," : "");
				xo_emit("{d:/%s} 4 PCI-e x1 slots{e:pci-express-slots/4}", 
				    comma ? "," : "");
				comma = 1;
			}
			if (fvec & (1 << 5)) {
				//printf("%s SATA RAID-0/1/10", comma ? "," : "");
				xo_emit("{d:/%s} SATA RAID-0/1/10{eq:feature-raid-0-1-10/true}", 
				    comma ? "," : "");
				comma = 1;
			}
			if (fvec & (1 << 3)) {
				//printf("%s SATA AHCI", comma ? "," : "");
				xo_emit("{d:/%s} SATA AHCI{eq:feature-sata-ahci/true}", 
				    comma ? "," : "");
				comma = 1;
			}
		}
	}
}

static void
cap_debug(int fd, struct pci_conf *p, uint8_t ptr)
{
	uint16_t debug_port;

	xo_emit("{e:capability-name/Debug}");
	debug_port = read_config(fd, &p->pc_sel, ptr + PCIR_DEBUG_PORT, 2);
	//printf("EHCI Debug Port at offset 0x%x in map 0x%x", debug_port &
	xo_emit("EHCI Debug Port at offset {:ehci-debug-port-offset/0x%x}"
	    " in map {:ehci-debug-port-map/0x%x}", debug_port &
	    PCIM_DEBUG_PORT_OFFSET, PCIR_BAR(debug_port >> 13));
}

static void
cap_subvendor(int fd, struct pci_conf *p, uint8_t ptr)
{
	uint32_t id;

	xo_emit("{eq:capability-name/Subvendor}");
	id = read_config(fd, &p->pc_sel, ptr + PCIR_SUBVENDCAP_ID, 4);
	//printf("PCI Bridge card=0x%08x", id);
	xo_emit("PCI Bridge card={:pci-bridge-subdevice-id/0x%04x}", 
	    (id >> 16) & 0xffff);
	xo_emit("{:pci-bridge-subvendor-id/%04x/0x%04x}", 
	    id & 0xffff);
}

#define	MAX_PAYLOAD(field)		(128 << (field))

static const char *
link_speed_string(uint8_t speed)
{

	switch (speed) {
	case 1:
		return ("2.5");
	case 2:
		return ("5.0");
	case 3:
		return ("8.0");
	default:
		return ("undef");
	}
}

static const char *
aspm_string(uint8_t aspm)
{

	switch (aspm) {
	case 1:
		return ("L0s");
	case 2:
		return ("L1");
	case 3:
		return ("L0s/L1");
	default:
		return ("disabled");
	}
}

static void
cap_express(int fd, struct pci_conf *p, uint8_t ptr)
{
	uint32_t cap, cap2;
	uint16_t ctl, flags, sta;

	flags = read_config(fd, &p->pc_sel, ptr + PCIER_FLAGS, 2);
	//printf("PCI-Express %d ", flags & PCIEM_FLAGS_VERSION);
	xo_emit("{:capability-name/PCI-Express} ");
	xo_emit("{:pci-express-version/%d} ", flags & PCIEM_FLAGS_VERSION);
	switch (flags & PCIEM_FLAGS_TYPE) {
	case PCIEM_TYPE_ENDPOINT:
		//printf("endpoint");
		xo_emit("{:device-type/endpoint}");
		break;
	case PCIEM_TYPE_LEGACY_ENDPOINT:
		//printf("legacy endpoint");
		xo_emit("{:device-type/legacy endpoint}");
		break;
	case PCIEM_TYPE_ROOT_PORT:
		//printf("root port");
		xo_emit("{:device-type/root port}");
		break;
	case PCIEM_TYPE_UPSTREAM_PORT:
		//printf("upstream port");
		xo_emit("{:device-type/upstream port}");
		break;
	case PCIEM_TYPE_DOWNSTREAM_PORT:
		//printf("downstream port");
		xo_emit("{:device-type/downstream port}");
		break;
	case PCIEM_TYPE_PCI_BRIDGE:
		//printf("PCI bridge");
		xo_emit("{:device-type/PCI bridge}");
		break;
	case PCIEM_TYPE_PCIE_BRIDGE:
		//printf("PCI to PCIe bridge");
		xo_emit("{:device-type/PCI to PCIe bridge}");
		break;
	case PCIEM_TYPE_ROOT_INT_EP:
		//printf("root endpoint");
		xo_emit("{:device-type/root endpoint}");
		break;
	case PCIEM_TYPE_ROOT_EC:
		//printf("event collector");
		xo_emit("{:device-type/event collector}");
		break;
	default:
		//printf("type %d", (flags & PCIEM_FLAGS_TYPE) >> 4);
		xo_emit("type {:device-type/%d}", (flags & PCIEM_FLAGS_TYPE) >> 4);
		break;
	}
	if (flags & PCIEM_FLAGS_SLOT)
		//printf(" slot");
		xo_emit(" slot{eq:slot-implemented/true}");
	if (flags & PCIEM_FLAGS_IRQ)
		//printf(" IRQ %d", (flags & PCIEM_FLAGS_IRQ) >> 9);
		xo_emit(" IRQ {:interrupt-message-number/%d}", 
		    (flags & PCIEM_FLAGS_IRQ) >> 9);
	cap = read_config(fd, &p->pc_sel, ptr + PCIER_DEVICE_CAP, 4);
	cap2 = read_config(fd, &p->pc_sel, ptr + PCIER_DEVICE_CAP2, 4);
	ctl = read_config(fd, &p->pc_sel, ptr + PCIER_DEVICE_CTL, 2);
	//printf(" max data %d(%d)",
	xo_emit(" max data {:controller-maximum-payload/%d}"
	    "({:capability-maximum-payload/%d})",
	    MAX_PAYLOAD((ctl & PCIEM_CTL_MAX_PAYLOAD) >> 5),
	    MAX_PAYLOAD(cap & PCIEM_CAP_MAX_PAYLOAD));
	if ((cap & PCIEM_CAP_FLR) != 0)
		//printf(" FLR");
		xo_emit(" FLR{eq:pci-express-capability-flr/true}");
	cap = read_config(fd, &p->pc_sel, ptr + PCIER_LINK_CAP, 4);
	sta = read_config(fd, &p->pc_sel, ptr + PCIER_LINK_STA, 2);
	//printf(" link x%d(x%d)", (sta & PCIEM_LINK_STA_WIDTH) >> 4,
	xo_emit(" link x{:link-width/%d}(x{:link-max-width/%d})", 
	    (sta & PCIEM_LINK_STA_WIDTH) >> 4,
	    (cap & PCIEM_LINK_CAP_MAX_WIDTH) >> 4);
	if ((cap & (PCIEM_LINK_CAP_MAX_WIDTH | PCIEM_LINK_CAP_ASPM)) != 0)
		//printf("\n                ");
		xo_emit("\n                ");
	if ((cap & PCIEM_LINK_CAP_MAX_WIDTH) != 0) {
		//printf(" speed %s(%s)", (sta & PCIEM_LINK_STA_WIDTH) == 0 ?
		xo_emit(" speed {n:link-speed/%s}({n:link-max-speed/%s})", 
		    (sta & PCIEM_LINK_STA_WIDTH) == 0 ?
		    "0.0" : link_speed_string(sta & PCIEM_LINK_STA_SPEED),
	    	    link_speed_string(cap & PCIEM_LINK_CAP_MAX_SPEED));
	}
	if ((cap & PCIEM_LINK_CAP_ASPM) != 0) {
		ctl = read_config(fd, &p->pc_sel, ptr + PCIER_LINK_CTL, 2);
		//printf(" ASPM %s(%s)", aspm_string(ctl & PCIEM_LINK_CTL_ASPMC),
		xo_emit(" ASPM {:controller-active-state-power-management/%s}"
		    "({:capability-active-state-power-management/%s})", 
		    aspm_string(ctl & PCIEM_LINK_CTL_ASPMC),
		    aspm_string((cap & PCIEM_LINK_CAP_ASPM) >> 10));
	}
	if ((cap2 & PCIEM_CAP2_ARI) != 0) {
		ctl = read_config(fd, &p->pc_sel, ptr + PCIER_DEVICE_CTL2, 4);
		//printf(" ARI %s",
		xo_emit(" ARI {:pci-express-alternate-requester-id/%s}}",
		    (ctl & PCIEM_CTL2_ARI) ? "enabled" : "disabled");
	}
}

static void
cap_msix(int fd, struct pci_conf *p, uint8_t ptr)
{
	uint32_t pba_offset, table_offset, val;
	int msgnum, pba_bar, table_bar;
	uint16_t ctrl;

	ctrl = read_config(fd, &p->pc_sel, ptr + PCIR_MSIX_CTRL, 2);
	msgnum = (ctrl & PCIM_MSIXCTRL_TABLE_SIZE) + 1;

	val = read_config(fd, &p->pc_sel, ptr + PCIR_MSIX_TABLE, 4);
	table_bar = PCIR_BAR(val & PCIM_MSIX_BIR_MASK);
	table_offset = val & ~PCIM_MSIX_BIR_MASK;

	val = read_config(fd, &p->pc_sel, ptr + PCIR_MSIX_PBA, 4);
	pba_bar = PCIR_BAR(val & PCIM_MSIX_BIR_MASK);
	pba_offset = val & ~PCIM_MSIX_BIR_MASK;

	//printf("MSI-X supports %d message%s%s\n", msgnum,
	xo_emit("{:capability-name/MSI-X} ");
	xo_emit("supports {:tyble-size/%d} message{d:/%s%s}\n", 
	    msgnum, (msgnum == 1) ? "" : "s",
	    (ctrl & PCIM_MSIXCTRL_MSIX_ENABLE) ? ", enabled" : "");
	xo_emit("{e:msix-enable/%s}", 
	    (ctrl & PCIM_MSIXCTRL_MSIX_ENABLE) ? "true" : "false");

	//printf("                 ");
	xo_emit("                 ");
	//printf("Table in map 0x%x[0x%x], PBA in map 0x%x[0x%x]",
	xo_emit("Table in map {:base-index-register/0x%x}"
	    "[{:table-offset/0x%x}], "
	    "PBA in map {:pba-index-register/0x%x}"
	    "[{:pba-offset/0x%x}]",
	    table_bar, table_offset, pba_bar, pba_offset);
}

static void
cap_sata(int fd, struct pci_conf *p, uint8_t ptr)
{

	//printf("SATA Index-Data Pair");
	xo_emit("{:capability-name/SATA} ");
	xo_emit("Index-Data Pair{eq:sata-index-data-pair/true}");
}

static void
cap_pciaf(int fd, struct pci_conf *p, uint8_t ptr)
{
	uint8_t cap;

	cap = read_config(fd, &p->pc_sel, ptr + PCIR_PCIAF_CAP, 1);
	//printf("PCI Advanced Features:%s%s",
	xo_emit("{:capability-name/PCI Advanced Features}:");
	xo_emit("{d:/%s%s}",
	    cap & PCIM_PCIAFCAP_FLR ? " FLR" : "",
	    cap & PCIM_PCIAFCAP_TP  ? " TP"  : "");
	xo_emit("{e:pci-avanced-feature-flr/%s}"
	    "{e:pci-advance-feature-tp/%s}",
	    cap & PCIM_PCIAFCAP_FLR ? "true" : "false",
	    cap & PCIM_PCIAFCAP_TP  ? "true"  : "false");
}

void
list_caps(int fd, struct pci_conf *p)
{
	int express;
	uint16_t sta;
	uint8_t ptr, cap;

	/* Are capabilities present for this device? */
	sta = read_config(fd, &p->pc_sel, PCIR_STATUS, 2);
	if (!(sta & PCIM_STATUS_CAPPRESENT))
		return;

	switch (p->pc_hdr & PCIM_HDRTYPE) {
	case PCIM_HDRTYPE_NORMAL:
	case PCIM_HDRTYPE_BRIDGE:
		ptr = PCIR_CAP_PTR;
		break;
	case PCIM_HDRTYPE_CARDBUS:
		ptr = PCIR_CAP_PTR_2;
		break;
	default:
		errx(1, "list_caps: bad header type");
	}

	/* Walk the capability list. */
	express = 0;
	ptr = read_config(fd, &p->pc_sel, ptr, 1);
	xo_open_list("capability");
	while (ptr != 0 && ptr != 0xff) {
		cap = read_config(fd, &p->pc_sel, ptr + PCICAP_ID, 1);
		//printf("    cap %02x[%02x] = ", cap, ptr);
		xo_open_instance("capability");
		xo_emit("    cap {:capability-number/%02x/0x%x}"
		    "[{:capability-pointer/%02x/0x%x}] = ", 
		    cap, ptr);
		switch (cap) {
		case PCIY_PMG:
			cap_power(fd, p, ptr);
			break;
		case PCIY_AGP:
			cap_agp(fd, p, ptr);
			break;
		case PCIY_VPD:
			cap_vpd(fd, p, ptr);
			break;
		case PCIY_MSI:
			cap_msi(fd, p, ptr);
			break;
		case PCIY_PCIX:
			cap_pcix(fd, p, ptr);
			break;
		case PCIY_HT:
			cap_ht(fd, p, ptr);
			break;
		case PCIY_VENDOR:
			cap_vendor(fd, p, ptr);
			break;
		case PCIY_DEBUG:
			cap_debug(fd, p, ptr);
			break;
		case PCIY_SUBVENDOR:
			cap_subvendor(fd, p, ptr);
			break;
		case PCIY_EXPRESS:
			express = 1;
			cap_express(fd, p, ptr);
			break;
		case PCIY_MSIX:
			cap_msix(fd, p, ptr);
			break;
		case PCIY_SATA:
			cap_sata(fd, p, ptr);
			break;
		case PCIY_PCIAF:
			cap_pciaf(fd, p, ptr);
			break;
		default:
			//printf("unknown");
			xo_emit("{:capability-name/unknown}");
			break;
		}
		xo_emit("\n");
		xo_close_instance("capability");
		ptr = read_config(fd, &p->pc_sel, ptr + PCICAP_NEXTPTR, 1);
	}
	xo_close_list("capability");

	if (express)
		list_ecaps(fd, p);
}

/* From <sys/systm.h>. */
static __inline uint32_t
bitcount32(uint32_t x)
{

	x = (x & 0x55555555) + ((x & 0xaaaaaaaa) >> 1);
	x = (x & 0x33333333) + ((x & 0xcccccccc) >> 2);
	x = (x + (x >> 4)) & 0x0f0f0f0f;
	x = (x + (x >> 8));
	x = (x + (x >> 16)) & 0x000000ff;
	return (x);
}

static void
ecap_aer(int fd, struct pci_conf *p, uint16_t ptr, uint8_t ver)
{
	uint32_t sta, mask;

	//printf("AER %d", ver);
	xo_emit("AER {eq:capability-name/Advanced Error Reporting}");
	xo_emit("{:advanced-error-reporting-version/%d}", ver);
	if (ver < 1)
		return;
	sta = read_config(fd, &p->pc_sel, ptr + PCIR_AER_UC_STATUS, 4);
	mask = read_config(fd, &p->pc_sel, ptr + PCIR_AER_UC_SEVERITY, 4);
	//printf(" %d fatal", bitcount32(sta & mask));
	xo_emit(" {:fatal-errors/%d} fatal", bitcount32(sta & mask));
	//printf(" %d non-fatal", bitcount32(sta & ~mask));
	xo_emit(" {:non-fatal-errors/%d} non-fatal", bitcount32(sta & ~mask));
	sta = read_config(fd, &p->pc_sel, ptr + PCIR_AER_COR_STATUS, 4);
	//printf(" %d corrected", bitcount32(sta));
	xo_emit(" {:corrected-errors/%d} corrected", bitcount32(sta));
}

static void
ecap_vc(int fd, struct pci_conf *p, uint16_t ptr, uint8_t ver)
{
	uint32_t cap1;

	//printf("VC %d", ver);
	xo_emit("VC {eq:capability-name/Virtual Channels}");
	xo_emit("{:virtual-channel-version/%d}", ver);
	if (ver < 1)
		return;
	cap1 = read_config(fd, &p->pc_sel, ptr + PCIR_VC_CAP1, 4);
	//printf(" max VC%d", cap1 & PCIM_VC_CAP1_EXT_COUNT);
	xo_emit(" max VC{:highest-virtual-channel-number/%d}", cap1 & PCIM_VC_CAP1_EXT_COUNT);
	if ((cap1 & PCIM_VC_CAP1_LOWPRI_EXT_COUNT) != 0)
		//printf(" lowpri VC0-VC%d",
		xo_emit(" lowpri VC0-VC{:highest-low-priority-virtual-channel-number/%d}",
		    (cap1 & PCIM_VC_CAP1_LOWPRI_EXT_COUNT) >> 4);
}

static void
ecap_sernum(int fd, struct pci_conf *p, uint16_t ptr, uint8_t ver)
{
	uint32_t high, low;

	//printf("Serial %d", ver);
	xo_emit("Serial {eq:capability-name/Serial Number}");
	xo_emit("{:serial-number-version/%d}", ver);
	if (ver < 1)
		return;
	low = read_config(fd, &p->pc_sel, ptr + PCIR_SERIAL_LOW, 4);
	high = read_config(fd, &p->pc_sel, ptr + PCIR_SERIAL_HIGH, 4);
	//printf(" %08x%08x", high, low);
	xo_emit(" {:serial-number/%08x%08x/0x%x%08x}", high, low);
}

static void
ecap_vendor(int fd, struct pci_conf *p, uint16_t ptr, uint8_t ver)
{
	uint32_t val;

	//printf("Vendor %d", ver);
	xo_emit("{:capability-name/Vendor} ");
	xo_emit("{:vendor-version/%d}", ver);
	if (ver < 1)
		return;
	val = read_config(fd, &p->pc_sel, ptr + 4, 4);
	//printf(" ID %d", val & 0xffff);
	xo_emit(" ID {:vendor-id/%d}", val & 0xffff);
}

static void
ecap_sec_pcie(int fd, struct pci_conf *p, uint16_t ptr, uint8_t ver)
{
	uint32_t val;

	//printf("PCIe Sec %d", ver);
	xo_emit("{eq:capability-name/Secondary Uncorrectable Errors}");
	xo_emit("PCIe Sec {:version/%d}", ver);
	if (ver < 1)
		return;
	val = read_config(fd, &p->pc_sel, ptr + 8, 4);
	//printf(" lane errors %#x", val);
	xo_emit(" lane errors {:lane-errors/%#x}", val);
}

struct {
	uint16_t id;
	const char *name;
} ecap_names[] = {
	{ PCIZ_PWRBDGT, "Power Budgeting" },
	{ PCIZ_RCLINK_DCL, "Root Complex Link Declaration" },
	{ PCIZ_RCLINK_CTL, "Root Complex Internal Link Control" },
	{ PCIZ_RCEC_ASSOC, "Root Complex Event Collector ASsociation" },
	{ PCIZ_MFVC, "MFVC" },
	{ PCIZ_RCRB, "RCRB" },
	{ PCIZ_ACS, "ACS" },
	{ PCIZ_ARI, "ARI" },
	{ PCIZ_ATS, "ATS" },
	{ PCIZ_SRIOV, "SRIOV" },
	{ PCIZ_MULTICAST, "Multicast" },
	{ PCIZ_RESIZE_BAR, "Resizable BAR" },
	{ PCIZ_DPA, "DPA" },
	{ PCIZ_TPH_REQ, "TPH Requester" },
	{ PCIZ_LTR, "LTR" },
	{ 0, NULL }
};

static void
list_ecaps(int fd, struct pci_conf *p)
{
	const char *name;
	uint32_t ecap;
	uint16_t ptr;
	int i;

	ptr = PCIR_EXTCAP;
	ecap = read_config(fd, &p->pc_sel, ptr, 4);
	if (ecap == 0xffffffff || ecap == 0)
		return;
	xo_open_list("extended-capability");
	for (;;) {
		xo_open_instance("extended-capability");
		//printf("    ecap %04x[%03x] = ", PCI_EXTCAP_ID(ecap), ptr);
		xo_emit("    ecap {:extended-capability-number/%04x/0x%x}"
		    "[{:capability-pointer/%03x/0x%x}] = ", 
		    PCI_EXTCAP_ID(ecap), ptr);
		switch (PCI_EXTCAP_ID(ecap)) {
		case PCIZ_AER:
			ecap_aer(fd, p, ptr, PCI_EXTCAP_VER(ecap));
			break;
		case PCIZ_VC:
			ecap_vc(fd, p, ptr, PCI_EXTCAP_VER(ecap));
			break;
		case PCIZ_SERNUM:
			ecap_sernum(fd, p, ptr, PCI_EXTCAP_VER(ecap));
			break;
		case PCIZ_VENDOR:
			ecap_vendor(fd, p, ptr, PCI_EXTCAP_VER(ecap));
			break;
		case PCIZ_SEC_PCIE:
			ecap_sec_pcie(fd, p, ptr, PCI_EXTCAP_VER(ecap));
			break;
		default:
			name = "unknown";
			for (i = 0; ecap_names[i].name != NULL; i++)
				if (ecap_names[i].id == PCI_EXTCAP_ID(ecap)) {
					name = ecap_names[i].name;
					break;
				}
			//printf("%s %d", name, PCI_EXTCAP_VER(ecap));
			xo_emit("{:extended-capability-type/%s} "
			    "{:extended-capability-version/%d}", 
			    name, PCI_EXTCAP_VER(ecap));
			break;
		}
		//printf("\n");
		xo_emit("\n");
		xo_close_instance("extended-capability");
		ptr = PCI_EXTCAP_NEXTPTR(ecap);
		if (ptr == 0)
			break;
		ecap = read_config(fd, &p->pc_sel, ptr, 4);
	}
	xo_close_list("extended-capability");
}

/* Find offset of a specific capability.  Returns 0 on failure. */
uint8_t
pci_find_cap(int fd, struct pci_conf *p, uint8_t id)
{
	uint16_t sta;
	uint8_t ptr, cap;

	/* Are capabilities present for this device? */
	sta = read_config(fd, &p->pc_sel, PCIR_STATUS, 2);
	if (!(sta & PCIM_STATUS_CAPPRESENT))
		return (0);

	switch (p->pc_hdr & PCIM_HDRTYPE) {
	case PCIM_HDRTYPE_NORMAL:
	case PCIM_HDRTYPE_BRIDGE:
		ptr = PCIR_CAP_PTR;
		break;
	case PCIM_HDRTYPE_CARDBUS:
		ptr = PCIR_CAP_PTR_2;
		break;
	default:
		return (0);
	}

	ptr = read_config(fd, &p->pc_sel, ptr, 1);
	while (ptr != 0 && ptr != 0xff) {
		cap = read_config(fd, &p->pc_sel, ptr + PCICAP_ID, 1);
		if (cap == id)
			return (ptr);
		ptr = read_config(fd, &p->pc_sel, ptr + PCICAP_NEXTPTR, 1);
	}
	return (0);
}

/* Find offset of a specific extended capability.  Returns 0 on failure. */
uint16_t
pcie_find_cap(int fd, struct pci_conf *p, uint16_t id)
{
	uint32_t ecap;
	uint16_t ptr;

	ptr = PCIR_EXTCAP;
	ecap = read_config(fd, &p->pc_sel, ptr, 4);
	if (ecap == 0xffffffff || ecap == 0)
		return (0);
	for (;;) {
		if (PCI_EXTCAP_ID(ecap) == id)
			return (ptr);
		ptr = PCI_EXTCAP_NEXTPTR(ecap);
		if (ptr == 0)
			break;
		ecap = read_config(fd, &p->pc_sel, ptr, 4);
	}
	return (0);
}
