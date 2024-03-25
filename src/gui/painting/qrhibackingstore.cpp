// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qrhibackingstore_p.h"

QT_BEGIN_NAMESPACE

QRhiBackingStore::QRhiBackingStore(QWindow *window)
    : QRasterBackingStore(window)
{
}

QRhiBackingStore::~QRhiBackingStore()
{
}

void QRhiBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(region);
    Q_UNUSED(offset);

    if (!rhi(window))
        createRhi(window);

    static QPlatformTextureList emptyTextureList;
    bool translucentBackground = m_image.hasAlphaChannel();
    rhiFlush(window, window->devicePixelRatio(), region, offset, &emptyTextureList, translucentBackground);
}

QT_END_NAMESPACE
