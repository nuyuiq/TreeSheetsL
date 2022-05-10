#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QString>
#include "image.h"



struct Document
{
    QString filename;
    Images imageslist;



    Document();
};

#endif // DOCUMENT_H
