#include "rtjpeg.h"
#include "../rtgui/multilangmgr.h"
#include "rtengine.h"
#include <cstdio>
#include <jerror.h>
#include <jpeglib.h>

namespace {

METHODDEF(void)
error_exit(j_common_ptr cinfo)
{
    (*cinfo->err->output_message)(cinfo);
    // jpeg_destroy(cinfo);
    throw rt_jpeg_error();
}

METHODDEF(void)
output_message(j_common_ptr cinfo)
{
    char buffer[JMSG_LENGTH_MAX];

    /* Create the message */
    (*cinfo->err->format_message)(cinfo, buffer);

    rt_jpeg_error_mgr *rterr =
        reinterpret_cast<rt_jpeg_error_mgr *>(cinfo->err);
    const char *filename = rterr->filename;

    if (rterr->pl) {
        rterr->pl->error(
            Glib::ustring::compose(M("JPEG_ERROR_MSG"), filename, buffer));
    } else {
        fprintf(stderr, "%s: %s\n", filename, buffer);
    }
}

} // namespace

GLOBAL(struct jpeg_error_mgr *)
rt_jpeg_std_error(rt_jpeg_error_mgr *err, const char *filename,
                  rtengine::ProgressListener *pl)
{
    jpeg_std_error(&(err->pub));
    err->pub.error_exit = error_exit;
    err->pub.output_message = output_message;
    err->filename = filename ? filename : "<UNKNOWN>";
    err->pl = pl;

    return &(err->pub);
}
