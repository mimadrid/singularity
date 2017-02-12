/* 
 * Copyright (c) 2015-2017, Gregory M. Kurtzer. All rights reserved.
 * 
 * Copyright (c) 2016-2017, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 * 
 * This software is licensed under a customized 3-clause BSD license.  Please
 * consult LICENSE file distributed with the sources of this project regarding
 * your rights to use or distribute this software.
 * 
 * NOTICE.  This Software was developed under funding from the U.S. Department of
 * Energy and the U.S. Government consequently retains certain rights. As such,
 * the U.S. Government has been granted for itself and others acting on its
 * behalf a paid-up, nonexclusive, irrevocable, worldwide license in the Software
 * to reproduce, distribute copies to the public, prepare derivative works, and
 * perform publicly and display publicly, and to permit other to do so. 
 * 
*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <grp.h>
#include <pwd.h>
#include <libgen.h>

#include "util/file.h"
#include "util/util.h"
#include "util/config_parser.h"
#include "util/message.h"
#include "util/privilege.h"

#include "../image.h"


int _singularity_image_open(struct image_object *image, int open_flags) {

    if ( image->fd > 0 ) {
        singularity_message(ERROR, "Called _singularity_image_open() on an open image object: %d\n", image->fd);
        ABORT(255);
    }

    if ( ( is_dir(image->path) == 0 ) && ( open_flags & (O_RDWR|O_WRONLY) ) ) {
        open_flags &= ~(O_RDWR|O_WRONLY) | O_RDONLY;
    }

    singularity_message(DEBUG, "Opening file descriptor to image: %s\n", image->path);
    if ( ( image->fd = open(image->path, open_flags, 0755) ) < 0 ) {
        singularity_message(ERROR, "Could not open image %s: %s\n", image->path, strerror(errno));
        ABORT(255);
    }

    if ( singularity_config_get_bool(ALLOW_USER_CONTAINER) == 0 ) { 
        struct stat image_stat;
        struct passwd *owner_pw;
        const char *limit_container_owner = singularity_config_get_value(ALLOWED_CONTAINER_OWNERS);
        // TODO: Support a list for container owner*s*
        
        if ( fstat(image->fd, &image_stat) != 0 ) {
            singularity_message(ERROR, "Could not fstat() image file descriptor (%d): %s\n", image->fd, strerror(errno));
            ABORT(255);
        }

        if ( ( owner_pw = getpwnam(limit_container_owner) ) == NULL ) {
            singularity_message(ERROR, "Could not obtain user info for %s: %s\n", limit_container_owner, strerror(errno));
            ABORT(255);
        }

        if ( owner_pw->pw_uid != image_stat.st_uid ) {
            singularity_message(ERROR, "Singularity image is not owned by required user: %s\n", limit_container_owner);
            ABORT(255);
        }

    }

    return(0);
}

