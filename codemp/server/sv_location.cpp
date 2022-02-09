#include "server.h"
#include "kdtree.h"

namespace LocationTree {
#define MAX_LOCATION_CHARS (32)

	typedef struct {
		char	message[MAX_LOCATION_CHARS];
		team_t	teamowner;
		int		cs_index;
	} enhancedLocation_t;

	static kdtree *tree = NULL;
	static enhancedLocation_t data[MAX_LOCATIONS];
	static int numUnique = 0;

	void *DataPtr(int index) {
		if (index < 0 || index >= MAX_LOCATIONS)
			Com_Error(ERR_DROP, "Enhanced location index %d out of bounds", index);
		return &data[index];
	}

	int *NumUnique(void) {
		return &numUnique;
	}

	void Create(void) {
		Free();
		tree = kd_create(3);
	}

	void Free(void) {
		if (tree) {
			kd_free(tree);
			tree = NULL;
		}
		memset(data, 0, sizeof(data));
		numUnique = 0;
	}

	int Insertf(const float *pos, void *data) {
		return kd_insertf(tree, pos, data);
	}

	void *Nearestf(const float *origin) {
		return kd_nearestf(tree, origin);
	}

	void ResFree(void *set) {
		kd_res_free((kdres *)set);
	}
}
