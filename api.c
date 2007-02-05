/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * api.c - Flickr API calls
 *
 * Copyright (C) 2007, David Beckett http://purl.org/net/dajobe/
 * 
 * This file is licensed under the following three licenses as alternatives:
 *   1. GNU Lesser General Public License (LGPL) V2.1 or any newer version
 *   2. GNU General Public License (GPL) V2 or any newer version
 *   3. Apache License, V2.0 or any newer version
 * 
 * You may not use this file except in compliance with at least one of
 * the above three licenses.
 * 
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * complete terms and further detail along with the license texts for
 * the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
 * 
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#undef HAVE_STDLIB_H
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <flickcurl.h>
#include <flickcurl_internal.h>



#undef OFFLINE


/* Debugging only */
#ifdef OFFLINE
static void flickcurl_debug_set_uri(flickcurl* fc, const char* uri);
#endif


#ifdef OFFLINE
static void
flickcurl_debug_set_uri(flickcurl* fc, const char* uri)
{
  strcpy(fc->uri, uri);
}
#endif


/*
 **********************************************************************
 * Flickr API Calls
 **********************************************************************
 */

/* Flickr test echo */
int
flickcurl_test_echo(flickcurl* fc, const char* key, const char* value)
{
  const char * parameters[10][2];
  int count=0;
  xmlDocPtr doc=NULL;
  int rc=0;
  
  parameters[count][0]  = key;
  parameters[count++][1]= value;

  parameters[count][0]  = NULL;

  flickcurl_set_sig_key(fc, NULL);

  if(flickcurl_prepare(fc, "flickr.test.echo", parameters, count)) {
    rc=1;
    goto tidy;
  }

  doc=flickcurl_invoke(fc);
  if(!doc) {
    rc=1;
    goto tidy;
  }

  fprintf(stderr, "Flickr echo returned %d bytes\n", fc->total_bytes);
  
  tidy:
  
  return rc;
}


/* Flickr auth.getFullToken - turn a frob into an auth_token */
char*
flickcurl_auth_getFullToken(flickcurl* fc, const char* frob)
{
  const char * parameters[10][2];
  int count=0;
  char *auth_token=NULL;
  xmlDocPtr doc=NULL;
  xmlXPathContextPtr xpathCtx=NULL; 
  
  parameters[count][0]   = "mini_token";
  parameters[count++][1] = (char*)frob;

  parameters[count][0]   = NULL;

  flickcurl_set_sig_key(fc, "api_sig");

  if(flickcurl_prepare(fc, "flickr.auth.getFullToken", parameters, count))
    goto tidy;

#ifdef OFFLINE
  flickcurl_debug_set_uri(fc, "file:auth.getFullToken.xml");
#endif

  doc=flickcurl_invoke(fc);
  if(!doc)
    goto tidy;
  
  xpathCtx = xmlXPathNewContext(doc);
  if(xpathCtx) {
    auth_token=flickcurl_xpath_eval(fc, xpathCtx,
                                    (const xmlChar*)"/rsp/auth/token");
    xmlXPathFreeContext(xpathCtx);
  }

  tidy:

  return auth_token;
}


static const char* flickcurl_photo_field_label[PHOTO_FIELD_LAST+1]={
  "(none)",
  "dateuploaded",
  "farm",
  "isfavorite",
  "license",
  "originalformat",
  "rotation",
  "server",
  "dates_lastupdate",
  "dates_posted",
  "dates_taken",
  "dates_takengranularity",
  "description",
  "editability_canaddmeta",
  "editability_cancomment",
  "geoperms_iscontact",
  "geoperms_isfamily",
  "geoperms_isfriend",
  "geoperms_ispublic",
  "location_accuracy",
  "location_latitude",
  "location_longitude",
  "owner_location",
  "owner_nsid",
  "owner_realname",
  "owner_username",
  "title",
  "visibility_isfamily",
  "visibility_isfriend",
  "visibility_ispublic",
  "secret",
  "originalsecret"
};


const char*
flickcurl_get_photo_field_label(flickcurl_photo_field field)
{
  if(field <= PHOTO_FIELD_LAST)
    return flickcurl_photo_field_label[(int)field];
  return NULL;
}


static const char* flickcurl_field_value_type_label[VALUE_TYPE_LAST+1]={
  "(none)",
  "photo id",
  "photo URI",
  "unix time",
  "boolean",
  "dateTime",
  "float",
  "integer",
  "string",
  "uri"
};


const char*
flickcurl_get_field_value_type_label(flickcurl_field_value_type datatype)
{
  if(datatype <= VALUE_TYPE_LAST)
    return flickcurl_field_value_type_label[(int)datatype];
  return NULL;
}


static struct {
  const xmlChar* xpath;
  flickcurl_photo_field field;
  flickcurl_field_value_type type;
} photo_fields_table[PHOTO_FIELD_LAST + 3]={
  {
    (const xmlChar*)"/rsp/photo/@id",
    PHOTO_FIELD_none,
    VALUE_TYPE_PHOTO_ID,
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/urls/url[@type=\"photopage\"]",
    PHOTO_FIELD_none,
    VALUE_TYPE_PHOTO_URI
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/@dateuploaded",
    PHOTO_FIELD_dateuploaded,
    VALUE_TYPE_UNIXTIME
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/@farm",
    PHOTO_FIELD_farm,
    VALUE_TYPE_INTEGER
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/@isfavorite",
    PHOTO_FIELD_isfavorite,
    VALUE_TYPE_BOOLEAN
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/@license",
    PHOTO_FIELD_license,
    VALUE_TYPE_INTEGER
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/@originalformat",
    PHOTO_FIELD_originalformat,
    VALUE_TYPE_STRING
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/@rotation",
    PHOTO_FIELD_rotation,
    VALUE_TYPE_INTEGER
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/@server",
    PHOTO_FIELD_server,
    VALUE_TYPE_INTEGER
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/dates/@lastupdate",
    PHOTO_FIELD_dates_lastupdate,
    VALUE_TYPE_UNIXTIME
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/dates/@posted",
    PHOTO_FIELD_dates_posted,
    VALUE_TYPE_UNIXTIME
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/dates/@taken",
    PHOTO_FIELD_dates_taken,
    VALUE_TYPE_DATETIME
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/dates/@takengranularity",
    PHOTO_FIELD_dates_takengranularity,
    VALUE_TYPE_INTEGER
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/description",
    PHOTO_FIELD_description,
    VALUE_TYPE_STRING
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/editability/@canaddmeta",
    PHOTO_FIELD_editability_canaddmeta,
    VALUE_TYPE_BOOLEAN
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/editability/@cancomment",
    PHOTO_FIELD_editability_cancomment,
    VALUE_TYPE_BOOLEAN
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/geoperms/@iscontact",
    PHOTO_FIELD_geoperms_iscontact,
    VALUE_TYPE_BOOLEAN
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/geoperms/@isfamily",
    PHOTO_FIELD_geoperms_isfamily,
    VALUE_TYPE_BOOLEAN
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/geoperms/@isfriend",
    PHOTO_FIELD_geoperms_isfriend,
    VALUE_TYPE_BOOLEAN
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/geoperms/@ispublic",
    PHOTO_FIELD_geoperms_ispublic,
    VALUE_TYPE_BOOLEAN
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/location/@accuracy",
    PHOTO_FIELD_location_accuracy,
    VALUE_TYPE_INTEGER
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/location/@latitude",
    PHOTO_FIELD_location_latitude,
    VALUE_TYPE_FLOAT
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/location/@longitude",
    PHOTO_FIELD_location_longitude,
    VALUE_TYPE_FLOAT
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/owner/@location",
    PHOTO_FIELD_owner_location,
    VALUE_TYPE_STRING
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/owner/@nsid",
    PHOTO_FIELD_owner_nsid,
    VALUE_TYPE_STRING
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/owner/@realname",
    PHOTO_FIELD_owner_realname,
    VALUE_TYPE_STRING
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/owner/@username",
    PHOTO_FIELD_owner_username,
    VALUE_TYPE_STRING
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/title",
    PHOTO_FIELD_title,
    VALUE_TYPE_STRING
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/visibility/@isfamily",
    PHOTO_FIELD_visibility_isfamily,
    VALUE_TYPE_BOOLEAN
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/visibility/@isfriend",
    PHOTO_FIELD_visibility_isfriend,
    VALUE_TYPE_BOOLEAN
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/visibility/@ispublic",
    PHOTO_FIELD_visibility_ispublic,
    VALUE_TYPE_BOOLEAN
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/@secret",
    PHOTO_FIELD_secret,
    VALUE_TYPE_STRING
  }
  ,
  {
    (const xmlChar*)"/rsp/photo/@originalsecret",
    PHOTO_FIELD_originalsecret,
    VALUE_TYPE_STRING
  }
  ,
  { 
    NULL,
    0,
    0
  }
};


/* Get information about a photo */
flickcurl_photo*
flickcurl_photos_getInfo(flickcurl* fc, const char* photo_id)
{
  const char * parameters[10][2];
  int count=0;
  xmlDocPtr doc=NULL;
  xmlXPathContextPtr xpathCtx=NULL; 
  int expri;
  xmlXPathObjectPtr xpathObj=NULL;
  xmlNodeSetPtr nodes;
  const xmlChar* xpathExpr=NULL;
  flickcurl_photo* photo=NULL;
  int i;
  
  if(!fc->auth_token) {
    flickcurl_error(fc, "No auth_token for method flickr.photos.getInfo");
    return NULL;
  }


  parameters[count][0]  = "photo_id";
  parameters[count++][1]= photo_id;

  parameters[count][0]  = "token";
  parameters[count++][1]= fc->auth_token;

  parameters[count][0]  = NULL;


  flickcurl_set_sig_key(fc, "api_sig");

  if(flickcurl_prepare(fc, "flickr.photos.getInfo", parameters, count))
    goto tidy;

#ifdef OFFLINE
  flickcurl_debug_set_uri(fc, "file:photos_getInfo.xml");
#endif

  doc=flickcurl_invoke(fc);
  if(!doc)
    goto tidy;


  xpathCtx = xmlXPathNewContext(doc);
  if(!xpathCtx) {
    flickcurl_error(fc, "Failed to create XPath context for document");
    fc->failed=1;
    goto tidy;
  }

  photo=(flickcurl_photo*)calloc(sizeof(flickcurl_photo), 1);
  
  for(expri=0; photo_fields_table[expri].xpath; expri++) {
    char *string_value=flickcurl_xpath_eval(fc, xpathCtx, 
                                            photo_fields_table[expri].xpath);
    flickcurl_field_value_type datatype=photo_fields_table[expri].type;
    int int_value= -1;
    flickcurl_photo_field field=photo_fields_table[expri].field;
    time_t unix_time;
    
    if(!string_value) {
      photo->fields[field].string = NULL;
      photo->fields[field].integer= -1;
      photo->fields[field].type   = VALUE_TYPE_NONE;
      continue;
    }

    switch(datatype) {
      case VALUE_TYPE_PHOTO_ID:
        photo->id=string_value;
        string_value=NULL;
        datatype=VALUE_TYPE_NONE;
        break;

      case VALUE_TYPE_PHOTO_URI:
        photo->uri=string_value;
        string_value=NULL;
        datatype=VALUE_TYPE_NONE;
        break;

      case VALUE_TYPE_UNIXTIME:
      case VALUE_TYPE_DATETIME:
      
        if(datatype == VALUE_TYPE_UNIXTIME)
          unix_time=atoi(string_value);
        else
          unix_time=curl_getdate((const char*)string_value, NULL);
        
        if(unix_time >= 0) {
          char* new_value=flickcurl_unixtime_to_isotime(unix_time);
#if FLICKCURL_DEBUG > 1
          fprintf(stderr, "  date from: '%s' unix time %ld to '%s'\n",
                  value, (long)unix_time, new_value);
#endif
          free(string_value);
          string_value= new_value;
          int_value= unix_time;
          datatype=VALUE_TYPE_DATETIME;
        } else
          /* failed to convert, make it a string */
          datatype=VALUE_TYPE_STRING;
        break;
        
      case VALUE_TYPE_INTEGER:
      case VALUE_TYPE_BOOLEAN:
        int_value=atoi(string_value);
        break;
        
      case VALUE_TYPE_NONE:
      case VALUE_TYPE_STRING:
      case VALUE_TYPE_FLOAT:
      case VALUE_TYPE_URI:
        break;
    }

    photo->fields[field].string = string_value;
    photo->fields[field].integer= int_value;
    photo->fields[field].type   = datatype;

#if FLICKCURL_DEBUG > 1
    fprintf(stderr, "field %d with %s value: '%s' / %d\n",
            field, flickcurl_field_value_type_label[datatype], 
            string_value, int_value);
#endif
      
    if(fc->failed)
      goto tidy;
  }


  /* Now do tags */
  xpathExpr=(const xmlChar*)"/rsp/photo/tags/tag";
  xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);
  if(!xpathObj) {
    flickcurl_error(fc, "Unable to evaluate XPath expression \"%s\"", 
                    xpathExpr);
    fc->failed=1;
    goto tidy;
  }
  
  nodes=xpathObj->nodesetval;
  for(i=0; i < xmlXPathNodeSetGetLength(nodes); i++) {
    xmlNodePtr node=nodes->nodeTab[i];
    xmlAttr* attr;
    flickcurl_tag* t;
    
    if(node->type != XML_ELEMENT_NODE) {
      flickcurl_error(fc, "Got unexpected node type %d", node->type);
      fc->failed=1;
      break;
    }
    
    t=(flickcurl_tag*)calloc(sizeof(flickcurl_tag), 1);
    t->photo=photo;
    
    for(attr=node->properties; attr; attr=attr->next) {
      const char *attr_name=(const char*)attr->name;
      char *attr_value;
      
      attr_value=(char*)malloc(strlen((const char*)attr->children->content)+1);
      strcpy(attr_value, (const char*)attr->children->content);
      
      if(!strcmp(attr_name, "id"))
        t->id=attr_value;
      else if(!strcmp(attr_name, "author"))
        t->author=attr_value;
      else if(!strcmp(attr_name, "raw"))
        t->raw=attr_value;
      else if(!strcmp(attr_name, "machine_tag"))
        t->machine_tag=atoi(attr_value);
    }
    
    t->cooked=(char*)malloc(strlen((const char*)node->children->content)+1);
    strcpy(t->cooked, (const char*)node->children->content);
    
#if FLICKCURL_DEBUG > 1
    fprintf(stderr, "tag: id %s author %s raw '%s' cooked '%s'\n",
            t->id, t->author, t->raw, t->cooked);
#endif
    
    if(fc->tag_handler)
      fc->tag_handler(fc->tag_data, t);
    
    photo->tags[photo->tags_count++]=t;
  } /* for nodes */


 tidy:
  if(xpathCtx)
    xmlXPathFreeContext(xpathCtx);

  if(xpathObj)
    xmlXPathFreeObject(xpathObj);
  
  if(fc->failed)
    doc=NULL;

  return photo;
}
