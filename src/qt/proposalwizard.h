// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PROPOSALWIZARD_H
#define BITCOIN_QT_PROPOSALWIZARD_H

#include <qt/forms/ui_proposalwizard.h>

#include <QByteArray>
#include <QDialog>
#include <QString>

namespace interfaces {
class Node;
}

class WalletModel;

class ProposalWizard : public QDialog
{
    Q_OBJECT
public:
    explicit ProposalWizard(interfaces::Node& node, WalletModel* walletModel, QWidget* parent = nullptr);
    ~ProposalWizard();

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
    Ui::ProposalWizard* m_ui;

    // State
    QByteArray m_json;
    QString m_hex;
    QString m_fee_formatted;
    int m_requiredConfs{6};

    void buildJsonAndHex();
};

#endif // BITCOIN_QT_PROPOSALWIZARD_H
