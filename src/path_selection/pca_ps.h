#ifndef __PCA_PS__
#define __PCA_PS__
#include "../list.h"
#include "../flow_manager/route_enforcer.h"
#include "../resource_manager.h"
#include "../path_metrics/path_metric_interface.h"
#include "../util.h"
uint32_t pca_path_selection(List *resources, struct application_spec *as);
#endif
