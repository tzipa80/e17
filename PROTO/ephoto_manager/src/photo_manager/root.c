// vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2

#include "photo_manager_private.h"


static void _pm_root_monitor_cb(void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char *path);
static Eet_Data_Descriptor *_pm_root_albums_file_name_edd_new(Eet_Data_Descriptor *edd_file_name);

struct pm_root
{
    const char *path;
    Eina_List *albums;

    //generally  a sync is associated to a photo manager
    //that's why we allow the user to set a sync here
    Photo_Manager_Sync *sync;

    Ecore_File_Monitor *monitor;
    Photo_Manager_Configuration conf;
};

/*
 * Create a new root node. The callbacks arguments can be NULL if the folder
 * is not monitor.
 *
 * @param album_new_cb  Callback called by the monitoring when a new album appears
 * @param album_delete_cb  Callback called by the monitoring when a album is deleted
 * @param pm_delete_cb  Callback called by the monitoring when the root folder is deleted
 * @param photo_new_cb  Callback called by the monitoring when a new photo appears
 * @param photo_delete_cb  Callback called by the monitoring when a photo is deleted
 * @param photo_update_cb  Callback called by the monitoring when a photo is updated
 * @param user_data Data sent in the callbacks
 * @param Returns the new root struct
 */
PM_Root *pm_root_new(
        Photo_Manager_Album_New_Cb album_new_cb, Photo_Manager_Album_Delete_Cb album_delete_cb,
        Photo_Manager_Delete_Cb pm_delete_cb,
        Photo_Manager_Photo_New_Cb photo_new_cb, Photo_Manager_Photo_Delete_Cb photo_delete_cb,
        Photo_Manager_Photo_Update_Cb photo_update_cb,
        void *user_data)
{
    PM_Root *root = calloc(1,sizeof(PM_Root));
    root->conf.album_new_cb = album_new_cb;
    root->conf.album_delete_cb = album_delete_cb;
    root->conf.pm_delete_cb = pm_delete_cb;
    root->conf.photo_new_cb = photo_new_cb;
    root->conf.photo_delete_cb = photo_delete_cb;
    root->conf.photo_update_cb = photo_update_cb;
    root->conf.data = user_data;

    return root;
}

/**
 * @brief Free a root struct
 * @param root the root struct
 */
void pm_root_free(PM_Root **root)
{
    PM_Album *album;

    ASSERT_RETURN_VOID(root!=NULL);
    ASSERT_RETURN_VOID((*root)!=NULL);

    EINA_STRINGSHARE_DEL((*root)->path);
    EINA_LIST_FREE( (*root)->albums, album)
        pm_album_free(&album);
    pm_root_monitor_stop(*root);


    FREE(*root);
}

/**
 * Start to monitor the root folder
 *
 * @param root the root struct
 */
void pm_root_monitor_start(PM_Root *root)
{
    ASSERT_RETURN_VOID(root!=NULL);
    ASSERT_RETURN_VOID(root->monitor==NULL);

    root->monitor = ecore_file_monitor_add(pm_root_path_get(root), _pm_root_monitor_cb, root);
    ASSERT_RETURN_VOID(root->monitor!=NULL);
}

/**
 * Stop to monitor the root folder
 *
 * @param root the root struct
 */
void pm_root_monitor_stop(PM_Root *root)
{
    ASSERT_RETURN_VOID(root!=NULL);

    if(root->monitor)
        ecore_file_monitor_del(root->monitor);
}

#define FCT_NAME pm_root
#define STRUCT_TYPE PM_Root

SET(sync, Photo_Manager_Sync *)

GET(path, const char*)
GET(albums, Eina_List *)
GET(sync, Photo_Manager_Sync *)

#undef FCT_NAME
#undef STRUCT_TYPE

/*
 * Set the path
 *
 * @param root the root struct
 * @param path the new path
 */
void pm_root_path_set(PM_Root *root, const char *path)
{
    ASSERT_RETURN_VOID(root!=NULL);
    ASSERT_RETURN_VOID(path!=NULL);

    EINA_STRINGSHARE_DEL(root->path);

    char *_path = strdup(path);
    int len = strlen(_path) -1;
    if(_path[len] == '/')
        _path[len] = '\0';

    root->path = eina_stringshare_add(_path);
    FREE(_path);
}

/**
 * @brief
 * @param file_name The album file name. The string have to be in eina_stringshare
 */
PM_Album *pm_root_album_search_file_name(PM_Root *root, const char *file_name)
{
    Eina_List *l;
    PM_Album *album;
    ASSERT_RETURN(root!=NULL);
    ASSERT_RETURN(file_name!=NULL);

    EINA_LIST_FOREACH(root->albums, l, album)
    {
        if(pm_album_file_name_get(album) == file_name)
            return album;
    }

    return NULL;
}

/*
 * Add an album 
 *
 * @param root the root struct
 * @param album the albuw to add
 */
void pm_root_album_add(PM_Root *root, PM_Album *album)
{
    ASSERT_RETURN_VOID(root!=NULL);
    ASSERT_RETURN_VOID(album!=NULL);

    pm_album_root_set(album, root);
    root->albums = eina_list_append(root->albums, album);
}

/*
 * Remove an album
 *
 * @param root the root struct
 */
void pm_root_album_remove(PM_Root *root, PM_Album *album)
{
    ASSERT_RETURN_VOID(root!=NULL);
    ASSERT_RETURN_VOID(album!=NULL);

    pm_album_root_set(album, NULL);
    root->albums = eina_list_remove(root->albums, album);
}

/*
 * Print the list of album in stdout
 *
 * @param root the root struct
 */
void pm_root_print(PM_Root *root)
{
    Eina_List *l;
    PM_Album *album;

    ASSERT_RETURN_VOID(root!=NULL);

    printf("#######################################\n");
    printf("####### Path : %s\n",root->path);
    printf("#######################################\n");
    EINA_LIST_FOREACH(root->albums,l,album)
        pm_album_print(album);
}


/*
 * Load the list of albums from the Eet file.
 * Tnis list contains only the pant and file name of each album.
 * You should use it to load the albums with pm_root_eet_album_load()
 *
 * @param root the root struct
 * @return Returns a new root with the list of albums
 */
PM_Root *pm_root_eet_albums_load(PM_Root *root)
{
    Eet_Data_Descriptor *edd, *edd_album;
    Eet_File *f;
    PM_Root *data;
    char path[PATH_MAX];

    ASSERT_RETURN(root!=NULL);

    snprintf(path,PATH_MAX,"%s/"EET_FILE,pm_root_path_get(root));
    f = pm_file_manager_open(path);
    ASSERT_RETURN(f!=NULL);

    edd_album = _pm_album_file_name_edd_new();
    edd = _pm_root_albums_file_name_edd_new(edd_album);

    data = eet_data_read(f, edd, "/album_list");

    pm_file_manager_close(path);
    eet_data_descriptor_free(edd_album);
    eet_data_descriptor_free(edd);

    return data;
}

/*
 * Save the list of albums
 *
 * @param root the root struct
 */
int pm_root_eet_albums_save(PM_Root *root)
{
    int res;
    Eet_Data_Descriptor *edd, *edd_album;
    Eet_File *f;
    char path[PATH_MAX];

    ASSERT_RETURN(root!=NULL);

    snprintf(path,PATH_MAX,"%s/"EET_FILE,pm_root_path_get(root));
    f = pm_file_manager_open(path);
    ASSERT_RETURN(f!=NULL);

    edd_album = _pm_album_file_name_edd_new();
    edd = _pm_root_albums_file_name_edd_new(edd_album);

    res = eet_data_write(f, edd, "/album_list", root, 0);

    pm_file_manager_close(path);
    eet_data_descriptor_free(edd_album);
    eet_data_descriptor_free(edd);

    return res;
}

/*
 * Load an album from the eet file
 *
 * @param root the root struct
 * @param key the file name of the album to load
 * @return Returns the album or NULL if the album is not in the Eet file
 */
PM_Album *pm_root_eet_album_load(PM_Root *root, const char* key)
{
    Eet_Data_Descriptor *edd;
    Eet_File *f;
    PM_Album *data;
    char path[PATH_MAX], buf[PATH_MAX];

    ASSERT_RETURN(root!=NULL);
    ASSERT_RETURN(key!=NULL);

    snprintf(path,PATH_MAX,"%s/"EET_FILE,pm_root_path_get(root));
    f = pm_file_manager_open(path);
    ASSERT_RETURN(f!=NULL);

    edd = _pm_album_global_header_edd_new();

    if(key[0]!='/')
        snprintf(buf,PATH_MAX,"/album %s",key);
    else
        snprintf(buf,PATH_MAX,"%s",key);

    data = eet_data_read(f, edd, buf);

    pm_file_manager_close(path);
    eet_data_descriptor_free(edd);

    return data;
}

/*
 * Remove an album from the Eet file
 *
 * @param root the root struct
 * @param key the file name of the album to delete
 */
int pm_root_eet_album_remove(PM_Root *root, const char* key)
{
    Eet_File *f;
    char buf[PATH_MAX], path[PATH_MAX];

    ASSERT_RETURN(root!=NULL);
    ASSERT_RETURN(key!=NULL);


    snprintf(path, PATH_MAX, "%s/"EET_FILE, pm_root_path_get(root));
    f = pm_file_manager_open(path);
    ASSERT_RETURN(f!=NULL);

    if(key[0]!='/')
        snprintf(buf,PATH_MAX,"/album %s",key);
    else
        snprintf(buf,PATH_MAX,"%s",key);

    snprintf(buf, PATH_MAX, "/album %s", buf);

    eet_delete(f, buf);

    pm_file_manager_close(path);

    return 1;
}

static void _pm_root_monitor_cb(void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char *path)
{
    PM_Root *root = (PM_Root *)data;
    switch(event)
    {
        case ECORE_FILE_EVENT_CREATED_DIRECTORY:
            LOG_INFO("New album: %s", path);
            root->conf.album_new_cb(root->conf.data, root, path);
            break;
        case ECORE_FILE_EVENT_DELETED_DIRECTORY:
            LOG_INFO("Deleted album; %s", path);
            root->conf.album_delete_cb(root->conf.data, root, path);
            break;
        case ECORE_FILE_EVENT_DELETED_SELF:
            LOG_INFO("Deleted pm root: %s", path);
            root->conf.pm_delete_cb(root->conf.data, root);
            break;
        default: ;
            break;
    }
}

static Eet_Data_Descriptor * _pm_root_albums_file_name_edd_new(Eet_Data_Descriptor *edd_file_name)
{
    Eet_Data_Descriptor *edd;
    Eet_Data_Descriptor_Class eddc;

    EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, PM_Root);
    eddc.func.str_direct_alloc = NULL;
    eddc.func.str_direct_free = NULL;

    edd = eet_data_descriptor_file_new(&eddc);

    EET_DATA_DESCRIPTOR_ADD_LIST(edd, PM_Root, "albums", albums, edd_file_name);

    return edd;
}



Photo_Manager_Configuration pm_conf_get(PM_Root *root)
{
    return root->conf;
}


