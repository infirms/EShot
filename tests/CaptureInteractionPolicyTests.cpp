#include <QtTest>
#include "capture/CaptureInteractionPolicy.h"
#include "core/TranslationManager.h"

class CaptureInteractionPolicyTests : public QObject {
    Q_OBJECT
private slots:
    void resizeHandleReleasesActiveTool()
    {
        QVERIFY(shouldReleaseToolForResize(true, 3, 0));
        QVERIFY(!shouldReleaseToolForResize(false, 3, 0));
        QVERIFY(!shouldReleaseToolForResize(true, 0, 0));
    }

    void toolPersistenceDefaultsToNone()
    {
        QCOMPARE(initialAnnotationTool(false, 5, 0), 0);
        QCOMPARE(initialAnnotationTool(true, 5, 0), 5);
    }

    void annotationPersistenceLabelsExistInEveryLanguage()
    {
        for (int language = 0; language < TranslationManager::LangCount; ++language) {
            TranslationManager::setLanguage(static_cast<TranslationManager::Language>(language));
            QVERIFY(!TranslationManager::rememberLastAnnotationTool().isEmpty());
            QVERIFY(!TranslationManager::rememberLastAnnotationToolHint().isEmpty());
            QVERIFY(!TranslationManager::drawingTools().isEmpty());
            QVERIFY(!TranslationManager::bottomToolbarControls().isEmpty());
        }
        TranslationManager::setLanguage(TranslationManager::Turkish);
        QVERIFY(TranslationManager::rememberLastAnnotationTool().contains(QStringLiteral("ı")));
    }
};

QTEST_APPLESS_MAIN(CaptureInteractionPolicyTests)
#include "CaptureInteractionPolicyTests.moc"
