/*
 * libfdt - Flat Device Tree manipulation
 * Copyright (C) 2006 David Gibson, IBM Corporation.
 *
 * libfdt is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 *
 *  a) This library is free software; you can redistribute it and/or
 *     modify it under the terms of the GNU General Public License as
 *     published by the Free Software Foundation; either version 2 of the
 *     License, or (at your option) any later version.
 *
 *     This library is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public
 *     License along with this library; if not, write to the Free
 *     Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 *     MA 02110-1301 USA
 *
 * Alternatively,
 *
 *  b) Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *     1. Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *     2. Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *     CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *     INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *     MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *     CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *     SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *     NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *     HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *     CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *     OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *     EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <nautilus/fdt/fdt.h>
#include <nautilus/endian.h>

int fdt_check_header(const void *fdt)
{
	if (fdt_magic(fdt) == FDT_MAGIC) {
		/* Complete tree */
		if (fdt_version(fdt) < FDT_FIRST_SUPPORTED_VERSION)
			return -FDT_ERR_BADVERSION;
		if (fdt_last_comp_version(fdt) > FDT_LAST_SUPPORTED_VERSION)
			return -FDT_ERR_BADVERSION;
	} else if (fdt_magic(fdt) == FDT_SW_MAGIC) {
		/* Unfinished sequential-write blob */
		if (fdt_size_dt_struct(fdt) == 0)
			return -FDT_ERR_BADSTATE;
	} else {
		return -FDT_ERR_BADMAGIC;
	}

	return 0;
}

const void *fdt_offset_ptr(const void *fdt, int offset, unsigned int len)
{
	unsigned absoffset = offset + fdt_off_dt_struct(fdt);

	if ((absoffset < offset)
	    || ((absoffset + len) < absoffset)
	    || (absoffset + len) > fdt_totalsize(fdt))
		return NULL;

	if (fdt_version(fdt) >= 0x11)
		if (((offset + len) < offset)
		    || ((offset + len) > fdt_size_dt_struct(fdt)))
			return NULL;

	return fdt_offset_ptr_(fdt, offset);
}

uint32_t fdt_next_tag(const void *fdt, int startoffset, int *nextoffset)
{
	const fdt32_t *tagp, *lenp;
	uint32_t tag;
	int offset = startoffset;
	const char *p;

	*nextoffset = -FDT_ERR_TRUNCATED;
	tagp = fdt_offset_ptr(fdt, offset, FDT_TAGSIZE);
	if (!tagp)
		return FDT_END; /* premature end */
	tag = fdt32_to_cpu(*tagp);
	offset += FDT_TAGSIZE;

	*nextoffset = -FDT_ERR_BADSTRUCTURE;
	switch (tag) {
	case FDT_BEGIN_NODE:
		/* skip name */
		do {
			p = fdt_offset_ptr(fdt, offset++, 1);
		} while (p && (*p != '\0'));
		if (!p)
			return FDT_END; /* premature end */
		break;

	case FDT_PROP:
		lenp = fdt_offset_ptr(fdt, offset, sizeof(*lenp));
		if (!lenp)
			return FDT_END; /* premature end */
		/* skip-name offset, length and value */
		offset += sizeof(struct fdt_property) - FDT_TAGSIZE
			+ fdt32_to_cpu(*lenp);
		if (fdt_version(fdt) < 0x10 && fdt32_to_cpu(*lenp) >= 8 &&
		    ((offset - fdt32_to_cpu(*lenp)) % 8) != 0)
			offset += 4;
		break;

	case FDT_END:
	case FDT_END_NODE:
	case FDT_NOP:
		break;

	default:
		return FDT_END;
	}

	if (!fdt_offset_ptr(fdt, startoffset, offset - startoffset))
		return FDT_END; /* premature end */

	*nextoffset = FDT_TAGALIGN(offset);
	return tag;
}

int fdt_check_node_offset_(const void *fdt, int offset)
{
	if ((offset < 0) || (offset % FDT_TAGSIZE)
	    || (fdt_next_tag(fdt, offset, &offset) != FDT_BEGIN_NODE))
		return -FDT_ERR_BADOFFSET;

	return offset;
}

int fdt_check_prop_offset_(const void *fdt, int offset)
{
	if ((offset < 0) || (offset % FDT_TAGSIZE)
	    || (fdt_next_tag(fdt, offset, &offset) != FDT_PROP))
		return -FDT_ERR_BADOFFSET;

	return offset;
}

int fdt_next_node(const void *fdt, int offset, int *depth)
{
	int nextoffset = 0;
	uint32_t tag;

	if (offset >= 0)
		if ((nextoffset = fdt_check_node_offset_(fdt, offset)) < 0)
			return nextoffset;

	do {
		offset = nextoffset;
		tag = fdt_next_tag(fdt, offset, &nextoffset);

		switch (tag) {
		case FDT_PROP:
		case FDT_NOP:
			break;

		case FDT_BEGIN_NODE:
			if (depth)
				(*depth)++;
			break;

		case FDT_END_NODE:
			if (depth && ((--(*depth)) < 0))
				return nextoffset;
			break;

		case FDT_END:
			if ((nextoffset >= 0)
			    || ((nextoffset == -FDT_ERR_TRUNCATED) && !depth))
				return -FDT_ERR_NOTFOUND;
			else
				return nextoffset;
		}
	} while (tag != FDT_BEGIN_NODE);

	return offset;
}

int fdt_first_subnode(const void *fdt, int offset)
{
	int depth = 0;

	offset = fdt_next_node(fdt, offset, &depth);
	if (offset < 0 || depth != 1)
		return -FDT_ERR_NOTFOUND;

	return offset;
}

int fdt_next_subnode(const void *fdt, int offset)
{
	int depth = 1;

	/*
	 * With respect to the parent, the depth of the next subnode will be
	 * the same as the last.
	 */
	do {
		offset = fdt_next_node(fdt, offset, &depth);
		if (offset < 0 || depth < 1)
			return -FDT_ERR_NOTFOUND;
	} while (depth > 1);

	return offset;
}

const char *fdt_find_string_(const char *strtab, int tabsize, const char *s)
{
	int len = strlen(s) + 1;
	const char *last = strtab + tabsize - len;
	const char *p;

	for (p = strtab; p <= last; p++)
		if (memcmp(p, s, len) == 0)
			return p;
	return NULL;
}

int fdt_move(const void *fdt, void *buf, int bufsize)
{
	FDT_CHECK_HEADER(fdt);

	if (fdt_totalsize(fdt) > bufsize)
		return -FDT_ERR_NOSPACE;

	memmove(buf, fdt, fdt_totalsize(fdt));
	return 0;
}

// this is our own addition to libfdt

int fdt_getreg(const void *fdt, int offset, fdt_reg_t *reg) {
	// from the device tree spec:
    // #size-cells and #address-cells are number of 32-bit cells that the
    // address and size data will be stored in for each reg prop
    // if not specified, assume: 2 for #address-cells, 1 for #size-cells

	int reg_lenp = 0;
    char *reg_prop = fdt_getprop(fdt, offset, "reg", &reg_lenp);

	void *address_cells_p = NULL;
	void *size_cells_p = NULL;
	int parent_iter_offset = offset;
	while (!address_cells_p || !size_cells_p) {
		// unclear if we should return here or if we should fall back to the default values
		// listed above (2, 1)
		if (parent_iter_offset < 0) {
			return -1;
		}
		int lenp = 0;

		address_cells_p = fdt_getprop(fdt, parent_iter_offset, "#address-cells", &lenp);
		size_cells_p = fdt_getprop(fdt, parent_iter_offset, "#size-cells", &lenp);

		parent_iter_offset = fdt_parent_offset(fdt, parent_iter_offset);
	}

	uint32_t address_cells = be32toh(*((uint32_t *)address_cells_p));
	uint32_t size_cells = be32toh(*((uint32_t *)size_cells_p));

	// printk("Addr: %d, size: %d\n", address_cells, size_cells);

	int i = 0;
	if (address_cells > 0) {
		if (address_cells == 1) reg->address = be32toh(*((uint32_t *)reg_prop));
		if (address_cells == 2) reg->address = be64toh(*((uint64_t *)reg_prop));
	}
	reg_prop += 4 * address_cells;
	if (size_cells > 0) {
		if (size_cells == 1) reg->size = be32toh(*((uint32_t *)reg_prop));
		if (size_cells == 2) reg->size = be64toh(*((uint64_t *)reg_prop));
	}

    // for (int i = 0; i < lenp / 8; i++) {
    //     printk("%x ", be64toh(((uint64_t *)reg_prop)[i]));
    // }
    // printk("\n");

	return 0;
}

int fdt_getreg_array(const void *fdt, int offset, fdt_reg_t *regs, int *num) {
 
	int reg_lenp = 0;
        char *reg_prop = fdt_getprop(fdt, offset, "reg", &reg_lenp);

	void *address_cells_p = NULL;
	void *size_cells_p = NULL;
	int parent_iter_offset = offset;
	while (!address_cells_p || !size_cells_p) {
		// unclear if we should return here or if we should fall back to the default values
		// listed above (2, 1)
		if (parent_iter_offset < 0) {
			return -1;
		}
		int lenp = 0;

		address_cells_p = fdt_getprop(fdt, parent_iter_offset, "#address-cells", &lenp);
		size_cells_p = fdt_getprop(fdt, parent_iter_offset, "#size-cells", &lenp);

		parent_iter_offset = fdt_parent_offset(fdt, parent_iter_offset);
	}

	uint32_t address_cells = be32toh(*((uint32_t *)address_cells_p));
	uint32_t size_cells = be32toh(*((uint32_t *)size_cells_p));

	// printk("Addr: %d, size: %d\n", address_cells, size_cells);

        int num_present = (reg_lenp) / ((4*address_cells) + (4*size_cells));
        *num = num_present > *num ? *num : num_present; // minimum

	for(int i = 0; i < *num; i++) {
	    if (address_cells > 0) {
		if (address_cells == 1) {
                  regs[i].address = be32toh(*((uint32_t *)reg_prop));
                }
		if (address_cells == 2) {
                  regs[i].address = be64toh(*((uint64_t *)reg_prop));
                }
	    }
	    reg_prop += 4 * address_cells;
	    if (size_cells > 0) {
		if (size_cells == 1) {
                  regs[i].size = be32toh(*((uint32_t *)reg_prop));
                }
		if (size_cells == 2) {
                  regs[i].size = be64toh(*((uint64_t *)reg_prop));
                }
	    }
            reg_prop += 4 * size_cells;
        }

    // for (int i = 0; i < lenp / 8; i++) {
    //     printk("%x ", be64toh(((uint64_t *)reg_prop)[i]));
    // }
    // printk("\n");

	return 0; 
}


off_t fdt_getreg_address(const void *fdt, int offset) {
	fdt_reg_t reg = { .address = 0, .size = 0 };
    int getreg_result = fdt_getreg(fdt, offset, &reg);

	return reg.address;
}

// TODO: this makes a lot of assumptions about the device:
// it only grabs the first interrupt, and it assumes that
// the #interrupt-cells property is 1 (32 bit)
uint32_t *fdt_get_interrupt(const void *fdt, int offset) {
	int lenp = 0;

	void *ints = fdt_getprop(fdt, offset, "interrupts", &lenp);
	uint32_t *vals = (uint32_t *)ints;

	return be32toh(vals[0]);
}

int print_device(const void *fdt, int offset, int depth) {
	int lenp = 0;
	char *name = fdt_get_name(fdt, offset, &lenp);
	char *compat_prop = fdt_getprop(fdt, offset, "compatible", &lenp);
	char *status = fdt_getprop(fdt, offset, "status", &lenp);
	// show tree depth with spaces
	for (int i = 0; i < depth; i++) {
		printk("  ");
	}
	printk("%s (%s - %s)\n", name, compat_prop, status);

	void *ints = fdt_getprop(fdt, offset, "interrupts", &lenp);
	uint32_t *vals = (uint32_t *)ints;
	for (int i = 0; i < lenp / 4; i++) {
		uint32_t val = be32toh(vals[i]);
		printk("\tInt: %d\n", val);
	}

	if (compat_prop && strcmp(compat_prop, "sifive,plic-1.0.0") == 0) {
		void *ints_extended_prop = fdt_getprop(fdt, offset, "interrupts-extended", &lenp);
		if (ints_extended_prop != NULL) {
			uint32_t *vals = (uint32_t *)ints_extended_prop;
			int context_count = lenp / 8;
			// printk("\tlenp: %d, context count %d\n",
			// lenp, context_count);
			for (int context = 0; context < context_count; context++) {
				int c_off = context * 2;
				int phandle = be32toh(vals[c_off]);
				int nr = be32toh(vals[c_off + 1]);
				if (nr != 9) {
					continue;
				}
				printk("\tcontext %d: (%d, %d)\n", context,
						phandle, nr);

				int intc_offset = fdt_node_offset_by_phandle(fdt, phandle);
				int cpu_offset = fdt_parent_offset(fdt, intc_offset);
				char *name = fdt_get_name(fdt, cpu_offset, &lenp);
				printk("\tcpu: %s\n", name);
			}
		}
	}

	return 1;
}

void print_fdt(const void *fdt) {
	fdt_walk_devices(fdt, print_device);
}

void fdt_walk_devices(const void *fdt, int (*callback)(const void *fdt, int offset, int depth)) {
	int depth = 0;
	int offset = 0;
	do {
		if (!callback(fdt, offset, depth)) {
			break;
		}

		offset = fdt_next_node(fdt, offset, &depth);
	} while (offset > 0);
}