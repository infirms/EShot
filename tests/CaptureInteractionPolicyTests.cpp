#include <QtTest>
#include <QSettings>
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

    void captureHintsOnlyAppearBeforeInteraction()
    {
        QVERIFY(shouldShowCaptureHints(true, false, false, false));
        QVERIFY(!shouldShowCaptureHints(false, false, false, false));
        QVERIFY(!shouldShowCaptureHints(true, true, false, false));
        QVERIFY(!shouldShowCaptureHints(true, false, true, false));
        QVERIFY(!shouldShowCaptureHints(true, false, false, true));
    }

    void captureHintStaysCenteredInsideTheActiveMonitor()
    {
        QCOMPARE(captureHintRect(QRect(1920, 0, 1920, 1080), QSize(640, 76)),
                 QRect(2560, 24, 640, 76));
        QCOMPARE(captureHintRect(QRect(0, 0, 480, 320), QSize(640, 76)),
                 QRect(16, 24, 448, 76));
    }

    void annotationPersistenceLabelsExistInEveryLanguage()
    {
        for (int language = 0; language < TranslationManager::LangCount; ++language) {
            TranslationManager::setLanguage(static_cast<TranslationManager::Language>(language), false);
            QVERIFY(!TranslationManager::rememberLastAnnotationTool().isEmpty());
            QVERIFY(!TranslationManager::rememberLastAnnotationToolHint().isEmpty());
            QVERIFY(!TranslationManager::drawingTools().isEmpty());
            QVERIFY(!TranslationManager::bottomToolbarControls().isEmpty());
        }
        TranslationManager::setLanguage(TranslationManager::Turkish, false);
        QVERIFY(TranslationManager::rememberLastAnnotationTool().contains(QStringLiteral("ı")));
    }

    void auditedTranslationsDoNotFallBackToEnglish()
    {
        const QList<QByteArray> keys = {
            "ocrLanguagePackMissing", "uploadAuthHelpYandex",
            "uploadAuthHelpGoogleDrive", "uploadAuthHelpApiKey",
            "uploadErrorYandexScopeMissing", "uploadErrorGoogleAuthFailed",
            "uploadErrorApiKeyMissing", "toolFontSize", "openFolder",
            "recordingStopShort", "recordingDetails", "ocrAutomatic",
            "captureHintDrag", "captureHintScreen", "captureHintRecording",
            "captureHintCopy", "captureHintSave", "captureHintCancel",
            "captureHintQuickSettings",
            "showCaptureHints", "showCaptureHintsTip"
        };
        TranslationManager::setLanguage(TranslationManager::English, false);
        QHash<QByteArray, QString> english;
        for (const QByteArray &key : keys) english.insert(key, TranslationManager::tr(key.constData()));
        for (int language = TranslationManager::German; language < TranslationManager::LangCount; ++language) {
            TranslationManager::setLanguage(static_cast<TranslationManager::Language>(language), false);
            for (const QByteArray &key : keys)
                QVERIFY2(TranslationManager::tr(key.constData()) != english.value(key), key.constData());
        }
    }
};

QTEST_APPLESS_MAIN(CaptureInteractionPolicyTests)
#include "CaptureInteractionPolicyTests.moc"
