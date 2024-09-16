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

    if (!rhi(window)) {
        QPlatformBackingStoreRhiConfig rhiConfig;
        switch (window->surfaceType()) {
        case QSurface::OpenGLSurface:
            rhiConfig.setApi(QPlatformBackingStoreRhiConfig::OpenGL);
            break;
        case QSurface::MetalSurface:
            rhiConfig.setApi(QPlatformBackingStoreRhiConfig::Metal);
            break;
        case QSurface::Direct3DSurface:
            rhiConfig.setApi(QPlatformBackingStoreRhiConfig::D3D11);
            break;
        case QSurface::VulkanSurface:
            rhiConfig.setApi(QPlatformBackingStoreRhiConfig::Vulkan);
            break;
        default:
            Q_UNREACHABLE();
        }

        rhiConfig.setEnabled(true);
        createRhi(window, rhiConfig);
    }

    static QPlatformTextureList emptyTextureList;
    bool translucentBackground = m_image.hasAlphaChannel();
    rhiFlush(window, window->devicePixelRatio(), region, offset, &emptyTextureList, translucentBackground);
}

QT_END_NAMESPACE
