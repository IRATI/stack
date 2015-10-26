#ifndef __VMPI_STATS_H__
#define __VMPI_STATS_H__

struct vmpi_stats {
        unsigned int rxres;
        unsigned int rxint;
        unsigned int txreq;
        unsigned int txres;
        unsigned int txint;
};

int vmpi_stats_init(struct vmpi_stats *);
void vmpi_stats_fini(struct vmpi_stats *);

#endif  /* __VMPI_STATS_H__ */
