#include "types.h"

extern int             strlen(const char*);
extern int             strncmp(const char*, const char*, uint);
extern int             printf(char*, ...) __attribute__ ((format (printf, 1, 2)));

struct fdt_header {
  uint32 magic;
  uint32 totalsize;
  uint32 off_dt_struct;
  uint32 off_dt_strings;
  uint32 off_mem_rsvmap;
  uint32 version;
  uint32 last_comp_version;
  uint32 boot_cpuid_phys;
  uint32 size_dt_strings;
  uint32 size_dt_struct;
};

#define FDT_MAGIC 0xd00dfeed
#define FDT_BEGIN_NODE 1
#define FDT_PROP 3
#define FDT_END 9

// Convert a big-endian 32-bit integer to little-endian
static inline uint32 be32_to_le32(uint32 val) {
  return ((val & 0xFF000000) >> 24) |
         ((val & 0x00FF0000) >> 8) |
         ((val & 0x0000FF00) << 8) |
         ((val & 0x000000FF) << 24);
}

// Convert a big-endian 64-bit integer to little-endian
static inline uint64 be64_to_le64(uint64 val) {
  return ((val & 0xFF00000000000000ULL) >> 56) |
         ((val & 0x00FF000000000000ULL) >> 40) |
         ((val & 0x0000FF0000000000ULL) >> 24) |
         ((val & 0x000000FF00000000ULL) >> 8) |
         ((val & 0x00000000FF000000ULL) << 8) |
         ((val & 0x0000000000FF0000ULL) << 24) |
         ((val & 0x000000000000FF00ULL) << 40) |
         ((val & 0x00000000000000FFULL) << 56);
}

/*
Parse the memory tag and the associated reg property.
Assumption: The 2nd address-size pair has the memory size passed to QEMU
QEMUOPTS = -machine virt -bios none -kernel $K/kernel -m 256M -smp $(CPUS) -nographic

memory@80000000 {
		device_type = "memory";
		reg = <0x00 0x80000000 0x00 0x10000000>;
	};

Used this to write the parser
https://devicetree-specification.readthedocs.io/en/stable/flattened-format.html
*/

uint64 parse_dtb_memory_size(uint64 dtb_addr)
{
    struct fdt_header *fdt = (struct fdt_header *)dtb_addr;

    // Convert header fields from big-endian to little-endian
    if (be32_to_le32(fdt->magic) != FDT_MAGIC)
    {
        return 0; // Invalid DTB
    }

    uint32 off_dt_strings = be32_to_le32(fdt->off_dt_strings);
    uint32 off_dt_struct = be32_to_le32(fdt->off_dt_struct);

    char *strings = (char *)dtb_addr + off_dt_strings;
    uint32 *structure = (uint32 *)(dtb_addr + off_dt_struct);

    while (be32_to_le32(*structure) != FDT_END) // FDT_END
    { 
        if (be32_to_le32(*structure) == FDT_BEGIN_NODE) // FDT_BEGIN_NODE
        { 
            structure++;
            char *node_name = (char *)structure;

            // printf("Node name: %s\n", node_name);

            // Check if this is the "memory" node
            if (strncmp(node_name, "memory", strlen("memory")) == 0)
            {
                // Skip node name and align to the next word boundary
                structure += (strlen(node_name) + sizeof(uint32)) / sizeof(uint32);

                // Iterate through all properties of the memory node
                while (be32_to_le32(*structure) == FDT_PROP) // FDT_PROP
                {
                    structure++; // Skip FDT_PROP token

                    uint32 len = be32_to_le32(*structure++);         // Property length
                    char *name = strings + be32_to_le32(*structure++); // Property name

                    if (strncmp(name, "reg", strlen("reg")) == 0 && len > 0)
                    {
                        // Handle reg property with multiple address-size pairs

                        // Calculate the number of 32-bit cells in the property value
                        int num_cells = len / sizeof(uint32);

                        uint64 base = 0;
                        uint64 size = 0;
                        
                        // Parse each address-size pair
                        // Assuming 1 cell for address and 1 for size
                        for (int i = 0; i < num_cells; i += 2) { 
                            base = be32_to_le32(structure[i]);
                            size = be32_to_le32(structure[i + 1]);
                            (void) base;

                            // printf("Address-Size Pair: Base Address: 0x%lx, Size: 0x%lx\n", base, size);
                        }
                        return size;
                    }

                    // Move to the next property
                    structure += (len + sizeof(uint32) - 1) / sizeof(uint32);
                }
            }
        }
        structure++;
    }

    return 0; // Memory node not found
}
