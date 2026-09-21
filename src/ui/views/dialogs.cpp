#include "ui/views/dialogs.h"

#include <QDate>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "core/repository.h"
#include "core/settings.h"
#include "ui/appcontext.h"
#include "ui/theme.h"
#include "ui/widgets.h"

namespace wordmem {

namespace {
QString tileQss() {
    // 统计/设置块底色：透明模式下改半透明白，与磨砂面板统一；普通模式保持浅灰实心。
    // 必须用 #dlgTile 而非 QFrame 选择器：QLabel 也是 QFrame 子类，类型选择器会级联
    // 给块内文字标签也刷上半透明底，多层叠加后文字区发白，故按 objectName 精确限定。
    return theme::ghostMode()
        ? QStringLiteral(
              "#dlgTile { background: rgba(255,255,255,80); border-radius:10px; }")
        : QStringLiteral("#dlgTile { background:#f4f7f8; border-radius:10px; }");
}
}  // namespace

// ============================================================ BaseDialog
BaseDialog::BaseDialog(const QString &title, QWidget *parent, const QSize &size,
                       bool popupClose)
    : QDialog(parent) {
    if (popupClose) {
        setWindowFlags(Qt::Popup | Qt::FramelessWindowHint |
                       Qt::NoDropShadowWindowHint);
        // 关闭弹层的那次点击不回放给下层控件，避免误触按钮
        setAttribute(Qt::WA_NoMouseReplay, true);
    } else {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setModal(true);
    }
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(size);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    panel = new QFrame(this);
    panel->setObjectName(QStringLiteral("panel"));
    // 透明模式下改为磨砂半透明面板，并加一圈淡边框保证在任意桌面上可辨识；
    // 普通模式保持纯白不透明。对话框每次新建后 exec()，构造期读取 ghost 状态即可。
    if (theme::ghostMode()) {
        panel->setStyleSheet(QStringLiteral(
            "#panel { background: %1; border: 1px solid %2; border-radius: 14px; }")
            .arg(theme::rgba(QStringLiteral("#ffffff"), 105),
                 theme::rgba(QStringLiteral("#58646e"), 70)));
    } else {
        panel->setStyleSheet(
            QStringLiteral("#panel { background: white; border-radius: 14px; }"));
    }
    outer->addWidget(panel);

    auto *panelLay = new QVBoxLayout(panel);
    panelLay->setContentsMargins(20, 14, 16, 18);
    panelLay->setSpacing(12);

    auto *head = new QHBoxLayout;
    auto *titleLabel = new QLabel(title, panel);
    titleLabel->setStyleSheet(
        QStringLiteral("color:%1; font-size:16px; font-weight:600;")
            .arg(QLatin1String(theme::INK_DARK)));
    auto *close = new IconButton(QStringLiteral("close"),
                                 QStringLiteral("#8a8f93"), 26, 13,
                                 QStringLiteral("关闭"), false, false, panel);
    connect(close, &QAbstractButton::clicked, this, &QDialog::reject);
    head->addWidget(titleLabel);
    head->addStretch(1);
    head->addWidget(close);
    panelLay->addLayout(head);

    body = new QVBoxLayout;
    body->setSpacing(10);
    panelLay->addLayout(body);
}

// ============================================================ StatsDialog
StatsDialog::StatsDialog(AppContext &ctx, QWidget *parent)
    : BaseDialog(QStringLiteral("学习统计"), parent, QSize(360, 340), true) {
    const StatsSummary stats = ctx.repo->statsSummary(QDate::currentDate());

    struct Tile {
        QString key;
        int value;
        const char *color;
    };
    const Tile tiles[6] = {
        {QStringLiteral("词书总词数"), stats.total, theme::INK_DARK},
        {QStringLiteral("已学习"), stats.learned, theme::GREEN},
        {QStringLiteral("学习中"), stats.learning, theme::INK_DARK},
        {QStringLiteral("已掌握"), stats.mastered, theme::GREEN},
        {QStringLiteral("今日待复习"), stats.due, theme::ORANGE},
        {QStringLiteral("今日已复习"), stats.reviewedToday, theme::INK_DARK},
    };
    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(10);
    for (int i = 0; i < 6; ++i)
        grid->addWidget(
            tile(tiles[i].key, tiles[i].value,
                 QLatin1String(tiles[i].color)),
            i / 2, i % 2);
    body->addLayout(grid);
    body->addSpacing(4);

    auto *cap = new QLabel(QStringLiteral("总体进度"), panel);
    cap->setStyleSheet(QStringLiteral("color:%1; font-size:13px; padding-left:2px;")
                           .arg(QLatin1String(theme::INK_SOFT)));
    body->addWidget(cap);
    auto *bar = new SmoothBar(true, QLatin1String(theme::GREEN_ICON), panel);
    bar->setFraction(static_cast<double>(stats.learned) / qMax(1, stats.total));
    bar->setFixedHeight(20);
    body->addWidget(bar);
    body->addStretch(1);
}

QWidget *StatsDialog::tile(const QString &key, int value, const QString &color) {
    // 浅色圆角统计块：大数值 + 小标签
    auto *tile = new QFrame;
    tile->setObjectName(QStringLiteral("dlgTile"));
    tile->setStyleSheet(tileQss());
    auto *lay = new QVBoxLayout(tile);
    lay->setContentsMargins(14, 10, 14, 9);
    lay->setSpacing(1);
    auto *v = new QLabel(QString::number(value), tile);
    v->setStyleSheet(
        QStringLiteral("color:%1; font-size:19px; font-weight:700;").arg(color));
    auto *k = new QLabel(key, tile);
    k->setStyleSheet(QStringLiteral("color:%1; font-size:12px;")
                         .arg(QLatin1String(theme::INK_SOFT)));
    lay->addWidget(v);
    lay->addWidget(k);
    return tile;
}

// ============================================================ SettingsDialog
SettingsDialog::SettingsDialog(AppContext &ctx, QWidget *parent)
    : BaseDialog(QStringLiteral("设置"), parent, QSize(360, 336), true),
      m_ctx(ctx) {
    const AppConfig &cfg = ctx.config();

    body->addWidget(rowTile(QStringLiteral("每组学习词数"),
                            stepper(5, 50, cfg.batchSize, &m_spin)));
    body->addWidget(rowTile(
        QStringLiteral("答错重现间隔（词数）"),
        stepper(1, 5, cfg.requeueGap, &m_gapSpin),
        QStringLiteral("答错后在该词数与其 +1 之间随机重现，如设 3 则随机 3~4 个词后再次出现")));

    m_autoPron = new Switch(panel);
    m_autoPron->setChecked(cfg.autoPronounce);
    body->addWidget(rowTile(QStringLiteral("出词时自动发音"), m_autoPron));

    auto *resetBtn = new QPushButton(QStringLiteral("恢复初始学习进度"), panel);
    resetBtn->setCursor(Qt::PointingHandCursor);
    resetBtn->setFixedHeight(34);
    resetBtn->setStyleSheet(QStringLiteral(
        "QPushButton { color:#b03a3a; border:1px solid #e5c4c4;"
        " border-radius:10px; font-size:13px; }"
        "QPushButton:hover { background:#fdf1f1; }"
        "QPushButton:pressed { background:#f8e4e4; }"));
    connect(resetBtn, &QPushButton::clicked, this, &SettingsDialog::resetProgress);
    body->addWidget(resetBtn);

    auto *applyBtn = new QPushButton(QStringLiteral("保存"), panel);
    applyBtn->setCursor(Qt::PointingHandCursor);
    applyBtn->setFixedHeight(38);
    applyBtn->setStyleSheet(QStringLiteral(
                                "QPushButton { color:white; background:%1;"
                                " border-radius:19px; font-size:15px;"
                                " font-weight:600; }"
                                "QPushButton:hover { background:%2; }")
                                .arg(QLatin1String(theme::GREEN),
                                     QLatin1String(theme::GREEN_ICON)));
    connect(applyBtn, &QPushButton::clicked, this, &SettingsDialog::save);
    body->addWidget(applyBtn);
}

QWidget *SettingsDialog::rowTile(const QString &text, QWidget *content,
                                 const QString &tooltip) {
    // 浅色圆角分组行：左侧说明文字 + 右侧控件
    auto *tile = new QFrame;
    tile->setObjectName(QStringLiteral("dlgTile"));
    tile->setStyleSheet(tileQss());
    auto *lay = new QHBoxLayout(tile);
    lay->setContentsMargins(14, 9, 12, 9);
    lay->setSpacing(8);
    auto *label = new QLabel(text, tile);
    label->setStyleSheet(QStringLiteral("color:%1; font-size:14px;")
                             .arg(QLatin1String(theme::INK)));
    lay->addWidget(label);
    lay->addStretch(1);
    lay->addWidget(content);
    if (!tooltip.isEmpty()) {
        tile->setToolTip(tooltip);
        for (QWidget *w : tile->findChildren<QWidget *>())
            w->setToolTip(tooltip);
    }
    return tile;
}

QWidget *SettingsDialog::stepper(int lo, int hi, int value, QSpinBox **outSpin) {
    // 「- 数值 +」步进器
    auto *spin = new QSpinBox;
    spin->setRange(lo, hi);
    spin->setValue(value);
    spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    spin->setAlignment(Qt::AlignCenter);
    spin->setFixedSize(54, 28);
    // 透明模式下步进器底色也转半透明，避免数字框成为一块实心白
    const QString spinBg = theme::ghostMode()
                               ? theme::rgba(QStringLiteral("#ffffff"), 110)
                               : QStringLiteral("white");
    spin->setStyleSheet(QStringLiteral(
        "QSpinBox { background:%1; border:1px solid #dbe3e6;"
        " border-radius:8px; color:#2f3e46; font-size:14px; font-weight:600; }"
        "QSpinBox:focus { border:1px solid #3fa26b; }").arg(spinBg));
    auto *minus = new IconButton(QStringLiteral("minus"),
                                 QLatin1String(theme::GREEN), 28, 12,
                                 QStringLiteral("减少"));
    auto *plus = new IconButton(QStringLiteral("plus"),
                                QLatin1String(theme::GREEN), 28, 12,
                                QStringLiteral("增加"));
    QObject::connect(minus, &QAbstractButton::clicked, spin,
                     [spin] { spin->stepBy(-1); });
    QObject::connect(plus, &QAbstractButton::clicked, spin,
                     [spin] { spin->stepBy(1); });

    auto *box = new QWidget;
    auto *lay = new QHBoxLayout(box);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(5);
    lay->addWidget(minus);
    lay->addWidget(spin);
    lay->addWidget(plus);
    if (outSpin != nullptr)
        *outSpin = spin;
    return box;
}

void SettingsDialog::save() {
    m_ctx.config().batchSize = m_spin->value();
    m_ctx.config().requeueGap = m_gapSpin->value();
    m_ctx.config().autoPronounce = m_autoPron->isChecked();
    m_ctx.configs->save();
    accept();
}

void SettingsDialog::resetProgress() {
    const auto ret = QMessageBox::question(
        this, QStringLiteral("确认"),
        QStringLiteral("确定要清空全部学习记录吗？此操作不可恢复。"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        m_ctx.repo->resetProgress();
        QMessageBox::information(this, QStringLiteral("完成"),
                                 QStringLiteral("学习进度已重置。"));
        accept();
    }
}

}  // namespace wordmem
