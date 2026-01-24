// Copyright (c) 2016-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/masternodelist.h>
#include <qt/forms/ui_masternodelist.h>

#include <coins.h>
#include <evo/deterministicmns.h>
#include <qt/clientmodel.h>
#include <qt/descriptiondialog.h>
#include <qt/guiutil.h>
#include <qt/guiutil_font.h>
#include <qt/masternodemodel.h>
#include <qt/walletmodel.h>

#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QResizeEvent>
#include <QShowEvent>

namespace {
constexpr int ADDRESS_MIN_WIDTH{220};
} // anonymous namespace

bool MasternodeListSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    // Check type filter
    if (m_type_filter != All) {
        QModelIndex idx = sourceModel()->index(source_row, MasternodeModel::TYPE, source_parent);
        int type = sourceModel()->data(idx, Qt::EditRole).toInt();
        // MnType::Regular = 0, MnType::Evo = 1
        if (m_type_filter == Regular && type != 0) {
            return false;
        }
        if (m_type_filter == Evo && type != 1) {
            return false;
        }
    }

    // Check banned filter
    if (m_hide_banned) {
        QModelIndex idx = sourceModel()->index(source_row, MasternodeModel::STATUS, source_parent);
        int banned = sourceModel()->data(idx, Qt::EditRole).toInt();
        if (banned != 0) {
            return false;
        }
    }

    // Check text filter
    if (!filterRegularExpression().pattern().isEmpty()) {
        bool matches = false;
        for (int col = 0; col < sourceModel()->columnCount(); ++col) {
            QModelIndex idx = sourceModel()->index(source_row, col, source_parent);
            QString data = sourceModel()->data(idx, Qt::DisplayRole).toString();
            if (data.contains(filterRegularExpression())) {
                matches = true;
                break;
            }
        }
        if (!matches) {
            return false;
        }
    }

    // Check "owned" filter
    if (m_show_owned_only) {
        QModelIndex idx = sourceModel()->index(source_row, MasternodeModel::PROTX_HASH, source_parent);
        QString proTxHash = sourceModel()->data(idx, Qt::DisplayRole).toString();
        if (m_my_mn_hashes.find(proTxHash) == m_my_mn_hashes.end()) {
            return false;
        }
    }

    return true;
}

MasternodeList::MasternodeList(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::MasternodeList),
    m_model(new MasternodeModel(this)),
    m_proxy_model(new MasternodeListSortFilterProxyModel(this))
{
    ui->setupUi(this);

    GUIUtil::setFont({ui->label_count,
                      ui->countLabelDIP3
                     }, {GUIUtil::g_font_registry.GetWeightBold(), 14});

    // Set up proxy model
    m_proxy_model->setSourceModel(m_model);
    m_proxy_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxy_model->setSortRole(Qt::EditRole);

    // Set up table view
    ui->tableViewMasternodes->setModel(m_proxy_model);
    ui->tableViewMasternodes->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tableViewMasternodes->verticalHeader()->setVisible(false);
    ui->tableViewMasternodes->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* header = ui->tableViewMasternodes->horizontalHeader();
    header->setStretchLastSection(false);
    connect(header, &QHeaderView::sectionResized, this, &MasternodeList::refreshColumnWidths);

    // Hide ProTx Hash column (used for internal lookup)
    ui->tableViewMasternodes->setColumnHidden(MasternodeModel::PROTX_HASH, true);

    // Hide PoSe column by default (since "Hide banned" is checked by default)
    ui->tableViewMasternodes->setColumnHidden(MasternodeModel::POSE, true);

    ui->checkBoxOwned->setEnabled(false);

    contextMenuDIP3 = new QMenu(this);
    contextMenuDIP3->addAction(tr("Copy ProTx Hash"), this, &MasternodeList::copyProTxHash_clicked);
    contextMenuDIP3->addAction(tr("Copy Collateral Outpoint"), this, &MasternodeList::copyCollateralOutpoint_clicked);

    QMenu* filterMenu = contextMenuDIP3->addMenu(tr("Filter by"));
    filterMenu->addAction(tr("Collateral Address"), this, &MasternodeList::filterByCollateralAddress);
    filterMenu->addAction(tr("Payout Address"), this, &MasternodeList::filterByPayoutAddress);
    filterMenu->addAction(tr("Owner Address"), this, &MasternodeList::filterByOwnerAddress);
    filterMenu->addAction(tr("Voting Address"), this, &MasternodeList::filterByVotingAddress);

    connect(ui->tableViewMasternodes, &QTableView::customContextMenuRequested, this, &MasternodeList::showContextMenuDIP3);
    connect(ui->tableViewMasternodes, &QTableView::doubleClicked, this, &MasternodeList::extraInfoDIP3_clicked);
    connect(m_proxy_model, &QSortFilterProxyModel::rowsInserted, this, &MasternodeList::updateFilteredCount);
    connect(m_proxy_model, &QSortFilterProxyModel::rowsRemoved, this, &MasternodeList::updateFilteredCount);
    connect(m_proxy_model, &QSortFilterProxyModel::modelReset, this, &MasternodeList::updateFilteredCount);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MasternodeList::updateDIP3ListScheduled);
    timer->start(1000);

    GUIUtil::updateFonts();

    refreshColumnWidths();
}

MasternodeList::~MasternodeList()
{
    delete ui;
}

void MasternodeList::setClientModel(ClientModel* model)
{
    this->clientModel = model;
    if (model) {
        // try to update list when masternode count changes
        connect(clientModel, &ClientModel::masternodeListChanged, this, &MasternodeList::handleMasternodeListChanged);
    }
}

void MasternodeList::setWalletModel(WalletModel* model)
{
    this->walletModel = model;
    ui->checkBoxOwned->setEnabled(model != nullptr);
}

void MasternodeList::showContextMenuDIP3(const QPoint& point)
{
    QModelIndex index = ui->tableViewMasternodes->indexAt(point);
    if (index.isValid()) {
        contextMenuDIP3->exec(QCursor::pos());
    }
}

void MasternodeList::handleMasternodeListChanged()
{
    mnListChanged = true;
}

void MasternodeList::updateDIP3ListScheduled()
{
    if (!clientModel || clientModel->node().shutdownRequested()) {
        return;
    }

    // To prevent high cpu usage update only once in MASTERNODELIST_FILTER_COOLDOWN_SECONDS seconds
    // after filter was last changed unless we want to force the update.
    if (fFilterUpdatedDIP3) {
        int64_t nSecondsToWait = nTimeFilterUpdatedDIP3 - GetTime() + MASTERNODELIST_FILTER_COOLDOWN_SECONDS;
        ui->countLabelDIP3->setText(tr("Please wait…") + " " + QString::number(nSecondsToWait));

        if (nSecondsToWait <= 0) {
            updateDIP3List();
            fFilterUpdatedDIP3 = false;
        }
    } else if (mnListChanged) {
        int64_t nMnListUpdateSecods = clientModel->masternodeSync().isBlockchainSynced() ? MASTERNODELIST_UPDATE_SECONDS : MASTERNODELIST_UPDATE_SECONDS * 10;
        int64_t nSecondsToWait = nTimeUpdatedDIP3 - GetTime() + nMnListUpdateSecods;

        if (nSecondsToWait <= 0) {
            updateDIP3List();
            mnListChanged = false;
        }
    }
}

void MasternodeList::updateDIP3List()
{
    if (!clientModel || clientModel->node().shutdownRequested()) {
        return;
    }

    auto [mnList, pindex] = clientModel->getMasternodeList();
    if (!pindex) return;
    auto projectedPayees = mnList->getProjectedMNPayees(pindex);

    if (projectedPayees.empty() && mnList->getValidMNsCount() > 0) {
        // GetProjectedMNPayees failed to provide results for a list with valid mns.
        // Keep current list and let it try again later.
        return;
    }

    std::map<uint256, CTxDestination> mapCollateralDests;

    {
        // Get all UTXOs for each MN collateral in one go so that we can reduce locking overhead for cs_main
        // We also do this outside of the below Qt list update loop to reduce cs_main locking time to a minimum
        mnList->forEachMN(/*only_valid=*/false, [&](const auto& dmn) {
            CTxDestination collateralDest;
            Coin coin;
            if (clientModel->node().getUnspentOutput(dmn.getCollateralOutpoint(), coin) &&
                ExtractDestination(coin.out.scriptPubKey, collateralDest)) {
                mapCollateralDests.emplace(dmn.getProTxHash(), collateralDest);
            }
        });
    }

    ui->countLabelDIP3->setText(tr("Updating…"));

    nTimeUpdatedDIP3 = GetTime();

    std::map<uint256, int> nextPayments;
    for (size_t i = 0; i < projectedPayees.size(); i++) {
        const auto& dmn = projectedPayees[i];
        nextPayments.emplace(dmn->getProTxHash(), mnList->getHeight() + (int)i + 1);
    }

    // Build list of entries
    MasternodeEntryList entries;

    mnList->forEachMN(/*only_valid=*/false, [&](const auto& dmn) {
        QString collateralStr = tr("UNKNOWN");
        auto collateralDestIt = mapCollateralDests.find(dmn.getProTxHash());
        if (collateralDestIt != mapCollateralDests.end()) {
            collateralStr = QString::fromStdString(EncodeDestination(collateralDestIt->second));
        }

        int nNextPayment = 0;
        auto nextPaymentIt = nextPayments.find(dmn.getProTxHash());
        if (nextPaymentIt != nextPayments.end()) {
            nNextPayment = nextPaymentIt->second;
        }

        // Get a copy of the MnEntry for the model
        auto mnEntry = mnList->getMN(dmn.getProTxHash());
        if (mnEntry) {
            entries.push_back(std::make_unique<MasternodeEntry>(
                clientModel, std::move(mnEntry), collateralStr, nNextPayment));
        }
    });

    // Update model
    m_model->reconcile(std::move(entries));
    refreshColumnWidths();

    // Update my masternodes filter if needed
    if (walletModel && ui->checkBoxOwned->isChecked()) {
        updateMyMasternodeHashes();
    }

    updateFilteredCount();
}

void MasternodeList::updateMyMasternodeHashes()
{
    if (!clientModel || !walletModel) {
        return;
    }

    auto [mnList, pindex] = clientModel->getMasternodeList();
    if (!pindex) return;

    std::set<COutPoint> setOutpts;
    for (const auto& outpt : walletModel->wallet().listProTxCoins()) {
        setOutpts.emplace(outpt);
    }

    std::set<QString> myHashes;
    mnList->forEachMN(/*only_valid=*/false, [&](const auto& dmn) {
        bool fMyMasternode = setOutpts.count(dmn.getCollateralOutpoint()) ||
                             walletModel->wallet().isSpendable(PKHash(dmn.getKeyIdOwner())) ||
                             walletModel->wallet().isSpendable(PKHash(dmn.getKeyIdVoting())) ||
                             walletModel->wallet().isSpendable(dmn.getScriptPayout()) ||
                             walletModel->wallet().isSpendable(dmn.getScriptOperatorPayout());
        if (fMyMasternode) {
            myHashes.insert(QString::fromStdString(dmn.getProTxHash().ToString()));
        }
    });

    m_proxy_model->setMyMasternodeHashes(myHashes);
    m_proxy_model->forceInvalidateFilter();
}

void MasternodeList::updateFilteredCount()
{
    ui->countLabelDIP3->setText(QString::number(m_proxy_model->rowCount()));
}

void MasternodeList::refreshColumnWidths()
{
    // Bail out if resize in progress or viewport is too small
    const int tableWidth = ui->tableViewMasternodes->viewport()->width();
    if (m_col_refresh || tableWidth <= 0) {
        return;
    }
    m_col_refresh = true;

    auto* header = ui->tableViewMasternodes->horizontalHeader();
    header->setMinimumSectionSize(0);

    // Set fixed columns to ResizeToContents to get natural widths
    for (int col = 0; col < MasternodeModel::_COUNT; ++col) {
        if (col != MasternodeModel::SERVICE && col != MasternodeModel::OPERATOR_REWARD) {
            header->setSectionResizeMode(col, QHeaderView::ResizeToContents);
        }
    }

    // Calculate width used by fixed columns (excluding SERVICE and OPERATOR_REWARD)
    const int availableWidth = [this, &header, &tableWidth]() {
        int fixedWidth = 0;
        for (int idx = 0; idx < MasternodeModel::_COUNT; ++idx) {
            if (idx != MasternodeModel::SERVICE && idx != MasternodeModel::OPERATOR_REWARD &&
                !ui->tableViewMasternodes->isColumnHidden(idx)) {
                fixedWidth += header->sectionSize(idx);
            }
        }
        return std::max(0, tableWidth - fixedWidth);
    }();

    // Temporarily set OPERATOR_REWARD to ResizeToContents to measure its natural width
    header->setSectionResizeMode(MasternodeModel::OPERATOR_REWARD, QHeaderView::ResizeToContents);
    const int operatorRewardContentWidth = header->sectionSize(MasternodeModel::OPERATOR_REWARD);

    // OPERATOR_REWARD gets what's left after SERVICE takes its minimum, clamped to [0, contentWidth]
    const int operatorRewardWidth = std::clamp<int>(availableWidth - ADDRESS_MIN_WIDTH, 0, operatorRewardContentWidth);
    const int serviceWidth = availableWidth - operatorRewardWidth;

    header->setSectionResizeMode(MasternodeModel::SERVICE, QHeaderView::Interactive);
    header->setSectionResizeMode(MasternodeModel::OPERATOR_REWARD, QHeaderView::Interactive);
    header->resizeSection(MasternodeModel::SERVICE, serviceWidth);
    header->resizeSection(MasternodeModel::OPERATOR_REWARD, operatorRewardWidth);

    m_col_refresh = false;
}

void MasternodeList::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    refreshColumnWidths();
}

void MasternodeList::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    refreshColumnWidths();
}

void MasternodeList::on_filterLineEditDIP3_textChanged(const QString& strFilterIn)
{
    m_proxy_model->setFilterRegularExpression(QRegularExpression(QRegularExpression::escape(strFilterIn),
                                                                  QRegularExpression::CaseInsensitiveOption));
    nTimeFilterUpdatedDIP3 = GetTime();
    fFilterUpdatedDIP3 = true;
    ui->countLabelDIP3->setText(tr("Please wait…") + " " + QString::number(MASTERNODELIST_FILTER_COOLDOWN_SECONDS));
}

void MasternodeList::on_comboBoxType_currentIndexChanged(int index)
{
    m_proxy_model->setTypeFilter(static_cast<MasternodeListSortFilterProxyModel::TypeFilter>(index));
    // Hide TYPE column when filtering by specific type (already known from filter)
    ui->tableViewMasternodes->setColumnHidden(MasternodeModel::TYPE, index != MasternodeListSortFilterProxyModel::All);
    refreshColumnWidths();
    m_proxy_model->forceInvalidateFilter();
    updateFilteredCount();
}

void MasternodeList::on_checkBoxOwned_stateChanged(int state)
{
    m_proxy_model->setShowOwnedOnly(state == Qt::Checked);
    if (state == Qt::Checked) {
        updateMyMasternodeHashes();
    }
    m_proxy_model->forceInvalidateFilter();
    updateFilteredCount();
}

void MasternodeList::on_checkBoxHideBanned_stateChanged(int state)
{
    const bool hideBanned = state == Qt::Checked;
    m_proxy_model->setHideBanned(hideBanned);
    ui->tableViewMasternodes->setColumnHidden(MasternodeModel::POSE, hideBanned);
    refreshColumnWidths();
    m_proxy_model->forceInvalidateFilter();
    updateFilteredCount();
}

const MasternodeEntry* MasternodeList::GetSelectedEntry()
{
    if (!m_model) {
        return nullptr;
    }

    QItemSelectionModel* selectionModel = ui->tableViewMasternodes->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();

    if (selected.count() == 0) return nullptr;

    // Map from proxy to source model
    QModelIndex proxyIndex = selected.at(0);
    QModelIndex sourceIndex = m_proxy_model->mapToSource(proxyIndex);

    return m_model->getEntryAt(sourceIndex);
}

void MasternodeList::extraInfoDIP3_clicked()
{
    const auto* entry = GetSelectedEntry();
    if (!entry) {
        return;
    }

    auto* dialog = new DescriptionDialog(
        tr("Details for Masternode %1").arg(entry->proTxHash()),
        entry->toHtml(),
        /*parent=*/this);
    dialog->resize(1100, 550);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void MasternodeList::copyProTxHash_clicked()
{
    const auto* entry = GetSelectedEntry();
    if (!entry) {
        return;
    }

    QApplication::clipboard()->setText(entry->proTxHash());
}

void MasternodeList::copyCollateralOutpoint_clicked()
{
    const auto* entry = GetSelectedEntry();
    if (!entry) {
        return;
    }

    QApplication::clipboard()->setText(entry->collateralOutpoint());
}

void MasternodeList::filterByCollateralAddress()
{
    const auto* entry = GetSelectedEntry();
    if (entry) {
        ui->filterLineEditDIP3->setText(entry->collateralAddress());
    }
}

void MasternodeList::filterByPayoutAddress()
{
    const auto* entry = GetSelectedEntry();
    if (entry) {
        ui->filterLineEditDIP3->setText(entry->payoutAddress());
    }
}

void MasternodeList::filterByOwnerAddress()
{
    const auto* entry = GetSelectedEntry();
    if (entry) {
        ui->filterLineEditDIP3->setText(entry->ownerAddress());
    }
}

void MasternodeList::filterByVotingAddress()
{
    const auto* entry = GetSelectedEntry();
    if (entry) {
        ui->filterLineEditDIP3->setText(entry->votingAddress());
    }
}
