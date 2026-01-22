// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PROPOSALCREATE_H
#define BITCOIN_QT_PROPOSALCREATE_H

#include <qt/forms/ui_proposalcreate.h>

#include <QByteArray>
#include <QDialog>
#include <QString>

namespace interfaces {
class Node;
}

class WalletModel;

class ProposalCreate : public QDialog
{
    Q_OBJECT
public:
    explicit ProposalCreate(interfaces::Node& node, WalletModel* walletModel, QWidget* parent = nullptr);
    ~ProposalCreate();

private Q_SLOTS:
    void onCreate();
    void onViewJson();
    void onViewPayload();

    void updateLabels();
    void updateDisplayUnit();
    void validateFields();

private:
    interfaces::Node& m_node;
    WalletModel* m_walletModel;
    Ui::ProposalCreate* m_ui;

    // State
    QByteArray m_json;
    QString m_hex;
    QString m_fee_formatted;
    int m_requiredConfs{6};

    void buildJsonAndHex();
};

#endif // BITCOIN_QT_PROPOSALCREATE_H
