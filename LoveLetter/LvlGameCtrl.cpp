////////////////////////////////////////////////
//File Name           : LvlGameCtrl.cpp
//2nd Model Name      : Game
//Description         : Implement the GameController that controls the game.
//Lastest Update Time : 2021/2/14 16:36
//Version             : 1.4
////////////////////////////////////////////////

#include "LvlGameCtrl.h"
#include "LvlGamePlayer.h"

/*
    ���ܣ���Ϸ���������캯��
    ���ݣ�������λ��ң�������Ϸ����״̬��
    ������parent-�����ڵ�ָ�룻sPlayerNum-�������
*/
LVL_GameCtrller::LVL_GameCtrller(QWidget *parent) : QWidget(parent), bIsProcessing(false)
{
    setGeometry(0, 0, LVL_GAME_WINDOW_WIDTH, LVL_GAME_WINDOW_HEIGHT);

    //��ʼ���ƶѺͲ�����
    pstDeckManager = new LVL_GameDeckManager(this);
    pstGuesser = new LVL_GameGuesser(this, LVL_GAME_GUESSER_POS);
    pstGuesser->hide();//���ز��ƽ���

    //��ʼ�����
    LVL_GamePlayer::sActivePlayerNum = 0;
    apstPlayers[0] = new LVL_GameHuman(this, LVL_GAME_HUMAN_SLOT, LVL_GAME_HUMAN_DISCARD);
    apstPlayers[1] = new LVL_GameAI(this, LVL_GAME_AI1_SLOT, LVL_GAME_AI1_DISCARD);
    apstPlayers[2] = new LVL_GameAI(this, LVL_GAME_AI2_SLOT, LVL_GAME_AI2_DISCARD);
    apstPlayers[3] = new LVL_GameAI(this, LVL_GAME_AI3_SLOT, LVL_GAME_AI3_DISCARD);
    apstPlayers[0]->LVL_GameSetPortrait(LVL_PIC_PLAYER_PORT, LVL_GAME_HUMAN_PORT_GEO);
    apstPlayers[1]->LVL_GameSetPortrait(LVL_PIC_AI1_PORT, LVL_GAME_AI1_PORT_GEO);
    apstPlayers[2]->LVL_GameSetPortrait(LVL_PIC_AI2_PORT, LVL_GAME_AI2_PORT_GEO);
    apstPlayers[3]->LVL_GameSetPortrait(LVL_PIC_AI3_PORT, LVL_GAME_AI3_PORT_GEO);

    //����״̬��״̬��
    QStateMachine *pqsmGameCtrl = new QStateMachine(this);//��Ϸ״̬��
    QState *apqsPlayers[LVL_GAME_MAXIMUM_PLAYER_NUM];
    for (int i = 0; i < LVL_GAME_MAXIMUM_PLAYER_NUM; i++) apqsPlayers[i] = new QState(pqsmGameCtrl);

    //��ȡ��Ϸ����
    const LVL_GameSetting &stSettings = LVL_GameSetting::LVL_SystemGetSettings();

    //״̬ת�ƺ��ź�����
    for (int i = 0; i < LVL_GAME_MAXIMUM_PLAYER_NUM; i++) {
        apqsPlayers[i]->addTransition(apstPlayers[i], &LVL_GamePlayer::sgnRoundEnded, apqsPlayers[(i + 1) % LVL_GAME_MAXIMUM_PLAYER_NUM]);//��һغ�ת��
        connect(apqsPlayers[i], &QState::entered, apstPlayers[i], &LVL_GamePlayer::LVL_GameStartRound);//��һغϿ�ʼ�ź�
        connect(apstPlayers[i], &LVL_GamePlayer::sgnWantDeal, [=](LVL_InterCard* pstCard) {//�����Ҫ�����ź�
            if (1 == LVL_GamePlayer::sActivePlayerNum) pqsmGameCtrl->stop();
            else if (0 == pstDeckManager->LVL_GameMakeDeal(apstPlayers[i], pstCard, false)) pqsmGameCtrl->stop();
        });
        connect(apstPlayers[i], &LVL_GamePlayer::sgnTarget, [this](LVL_GamePlayer *pstTarget) {//��ұ�ѡ���ź�
            if (NULL != pstTarget) pstTarget->LVL_GameSetPortraitPic(pstTarget->qstrPortraitName + LVL_PIC_TARGETED,
                pstTarget->qstrPortraitName + LVL_PIC_FOCUSED, pstTarget->qstrPortraitName + LVL_PIC_FOCUSED);
        });
        connect(apstPlayers[i], &LVL_GamePlayer::sgnUntarget, [this](LVL_GamePlayer *pstTarget) {//��ұ�ȡ��ѡ���ź�
            if (NULL != pstTarget && LVL_GamePlayer::Normal == pstTarget->LVL_GameGetPlayerState()) pstTarget->LVL_GameSetDefaultPortraitPic();
        });
        connect(apstPlayers[i], &LVL_GamePlayer::sgnPortraitClicked, qobject_cast<LVL_GameHuman*>(apstPlayers[0]), &LVL_GameHuman::LVL_GameClickPortrait);//���ͷ�����ź�

        //�ж���Ϸ������������Ҫ������߳��ֲ����أ���Ҫ����������һ����
        int sPlayerNum = stSettings.LVL_GameReadSetting<int>("Player Number");
        if ((2 == sPlayerNum && (1 == i || 3 == i)) || (3 == sPlayerNum && 2 == i)) {
            apstPlayers[i]->LVL_GameKickPlayer();
            apstPlayers[i]->pstPortrait->hide();
        }
        else pstDeckManager->LVL_GameMakeDeal(apstPlayers[i], apstPlayers[i]->pstOldCard, false);
    }

    connect(pqsmGameCtrl, &QStateMachine::stopped, this, &LVL_GameCtrller::LVL_GameCalculatePoint);

    pqsmGameCtrl->setInitialState(apqsPlayers[0]);
    pqsmGameCtrl->start();
}

/*
    ���ܣ�����ṩһ��������Ϊ���Ƽ��ܶ����Ŀ��
    ������enSelect-Ҫ�������ƣ�pstSelf-���Ƴ����ߵ�ָ��
    ����ֵ��LVL_GamePlayer*-Ŀ����ҵ�ָ�룬���û����ҿ�����ΪĿ�����ҪĿ�꣬�򷵻�NULL
*/
LVL_GamePlayer *LVL_GameCtrller::LVL_GameGetTarget(LVL_InterCard::LVL_InterCardType_E enSelect, LVL_GamePlayer *pstSelf) {
    if (NULL == pstSelf) exit(-1);
    if (false == LVL_InterCard::m_abNeedTarget[enSelect]) return NULL;

    LVL_GamePlayer *apstActivePlayer[LVL_GAME_MAXIMUM_PLAYER_NUM];
    int32_t sSize = 0;
    for (int i = 0; i < sizeof(apstPlayers)/sizeof(apstPlayers[0]); i++)
        if ((LVL_InterCard::prince == enSelect || pstSelf != apstPlayers[i]) && LVL_GamePlayer::Normal == apstPlayers[i]->LVL_GameGetPlayerState())//�������⣬����ѡ���Լ���ΪĿ��
            apstActivePlayer[sSize++] = apstPlayers[i];
    if (sSize == 0) return NULL;
    return apstActivePlayer[qrand() % sSize];
}

/*
    ���ܣ��������Ƽ���Ч��
    ������enType-Ҫ���Ŀ������ࣻpstSelf-���Ƴ����ߵ�ָ�룻pstTarget-�������ö����ָ��
    ����ֵ����
*/
void LVL_GameCtrller::LVL_GameTriggerEffect(LVL_InterCard::LVL_InterCardType_E enType, LVL_GamePlayer *pstSelf, LVL_GamePlayer *pstTarget) {
    if (NULL == pstSelf) exit(-1);
    if (true == LVL_InterCard::m_abNeedTarget[enType] && NULL == pstTarget) return;//���û�п��е�Ŀ�꣬�Ͳ�ִ�п���Ч��
    
    QEventLoop qelLoop;
    LVL_GamePlayer *pstShow = NULL;

    bIsProcessing = true;
    switch (enType) {

        //����
    case LVL_InterCard::guard: {
        LVL_InterCard::LVL_InterCardType_E enGuess;
        if (qobject_cast<LVL_GameHuman*>(pstSelf)) {
            pstGuesser->raise();  pstGuesser->show();
            connect(pstGuesser, &LVL_GameGuesser::sgnGuessFinished, [&](LVL_InterCard::LVL_InterCardType_E enReceive) {
                pstGuesser->hide();
                enGuess = enReceive;
                qelLoop.quit();
            });
            qelLoop.exec();
        }
        else enGuess = (LVL_InterCard::LVL_InterCardType_E)(qrand() % 7 + 2);
        LVL_InterCard *pstGuess = new LVL_InterCard(this, enGuess, true,
            QPoint((LVL_GAME_WINDOW_WIDTH - LVL_INTER_CARD_WIDTH) / 2, (LVL_GAME_WINDOW_HEIGHT - LVL_INTER_CARD_HEIGHT) / 2));
        pstGuess->show();
        QTimer::singleShot(LVL_GAME_WATCHING_MSEC, &qelLoop, &QEventLoop::quit);
        qelLoop.exec();
        delete pstGuess;
        if (enGuess == pstTarget->pstOldCard->LVL_InterGetCardType()) pstTarget->LVL_GameKickPlayer();
        break;
    }

        //��ʦ
    case LVL_InterCard::priest: {
        if (qobject_cast<LVL_GameHuman*>(pstSelf)) {
            QSequentialAnimationGroup* pqsagWatching = new QSequentialAnimationGroup(this);
            pqsagWatching->addAnimation(pstTarget->pstOldCard->LVL_InterCreateFilppingAnimation(LVL_GAME_FLIPPING_MSEC));
            pqsagWatching->addAnimation(new QPauseAnimation(LVL_GAME_WATCHING_MSEC, this));
            pqsagWatching->addAnimation(pstTarget->pstOldCard->LVL_InterCreateFilppingAnimation(LVL_GAME_FLIPPING_MSEC));
            connect(pqsagWatching, &QAbstractAnimation::finished, &qelLoop, &QEventLoop::quit);
            pqsagWatching->start(QAbstractAnimation::DeleteWhenStopped);
            qelLoop.exec();
        }
        break;
    }

        //�о�
    case LVL_InterCard::baron: {

        //��ʾ����
        if (qobject_cast<LVL_GameHuman*>(pstSelf) || qobject_cast<LVL_GameHuman*>(pstTarget)) {
            pstShow = (qobject_cast<LVL_GameAI*>(pstSelf)) ? pstSelf : pstTarget;
            QSequentialAnimationGroup* pqsagWatching = new QSequentialAnimationGroup(this);
            pqsagWatching->addAnimation(pstShow->pstOldCard->LVL_InterCreateFilppingAnimation(LVL_GAME_FLIPPING_MSEC));
            pqsagWatching->addAnimation(new QPauseAnimation(LVL_GAME_WATCHING_MSEC, this));
            pqsagWatching->start(QAbstractAnimation::DeleteWhenStopped);
            connect(pqsagWatching, &QAbstractAnimation::finished, &qelLoop, &QEventLoop::quit);
            qelLoop.exec();
        }

        //��ƴ���Ƶ���
        if (pstSelf->pstOldCard->LVL_InterGetCardType() > pstTarget->pstOldCard->LVL_InterGetCardType()) pstTarget->LVL_GameKickPlayer();
        else if (pstSelf->pstOldCard->LVL_InterGetCardType() < pstTarget->pstOldCard->LVL_InterGetCardType()) pstSelf->LVL_GameKickPlayer();

        //��������
        if (NULL != pstShow && LVL_GamePlayer::Out != pstShow->LVL_GameGetPlayerState()) {
            QAbstractAnimation* pqaaSlipping = pstShow->pstOldCard->LVL_InterCreateFilppingAnimation(LVL_GAME_FLIPPING_MSEC);
            pqaaSlipping->start(QAbstractAnimation::DeleteWhenStopped);
            connect(pqaaSlipping, &QAbstractAnimation::finished, &qelLoop, &QEventLoop::quit);
            qelLoop.exec();
        }
        break;
    }
        //Ů��
    case LVL_InterCard::handmaid: pstSelf->LVL_GameGuardPlayer(); break;

        //����
    case LVL_InterCard::prince: {

        //��ʾ����
        QParallelAnimationGroup* pqsagThrowing = new QParallelAnimationGroup(this);
        QPoint qpOrigin = pstTarget->pstOldCard->pos();
        QPropertyAnimation *pqaaMoving = pstTarget->pstOldCard->LVL_InterCreateAnimation("pos", 1000, qpOrigin, qpOrigin + QPoint(0, 50));
        pqaaMoving->setKeyValueAt(0.3, qpOrigin - QPoint(0, 30));
        QAbstractAnimation *pqaaFading = pstTarget->pstOldCard->LVL_InterCreateAnimation("opacity", 1000, 1, 0, QEasingCurve::Linear);
        pqsagThrowing->addAnimation(pqaaMoving);
        pqsagThrowing->addAnimation(pqaaFading);
        pqsagThrowing->start(QAbstractAnimation::DeleteWhenStopped);
        connect(pqsagThrowing, &QAbstractAnimation::finished, &qelLoop, &QEventLoop::quit);
        qelLoop.exec();

        //�ָ�����ԭ����
        pstTarget->pstOldCard->setProperty("pos", qpOrigin);
        pstTarget->pstOldCard->setProperty("opacity", 1);

        if (LVL_InterCard::princess == pstTarget->pstOldCard->LVL_InterGetCardType()) pstTarget->LVL_GameKickPlayer();//�ж����������Ƿ�Ϊ����
        pstTarget->LVL_GameMakeDiscard(pstTarget->pstOldCard);
        pstDeckManager->LVL_GameMakeDeal(pstTarget, pstTarget->pstOldCard, true);
        break;
    }

        //����
    case LVL_InterCard::king: {

        //��ʾ����
        if (qobject_cast<LVL_GameHuman*>(pstSelf) || qobject_cast<LVL_GameHuman*>(pstTarget)) {
            pstShow = (qobject_cast<LVL_GameAI*>(pstSelf)) ? pstSelf : pstTarget;
            QSequentialAnimationGroup* pqsagWatching = new QSequentialAnimationGroup(this);
            pqsagWatching->addAnimation(pstShow->pstOldCard->LVL_InterCreateFilppingAnimation(LVL_GAME_FLIPPING_MSEC));
            pqsagWatching->addAnimation(new QPauseAnimation(LVL_GAME_WATCHING_MSEC, this));
            pqsagWatching->start(QAbstractAnimation::DeleteWhenStopped);
            connect(pqsagWatching, &QAbstractAnimation::finished, &qelLoop, &QEventLoop::quit);
            qelLoop.exec();
        }

        //��������
        LVL_InterCard::LVL_InterCardType_E enTemp = pstSelf->pstOldCard->LVL_InterGetCardType();
        pstSelf->LVL_GameSetCard(pstSelf->pstOldCard, pstTarget->pstOldCard->LVL_InterGetCardType());
        pstTarget->LVL_GameSetCard(pstTarget->pstOldCard, enTemp);

        //�ƶ�����
        QPoint qpTemp = pstSelf->pstOldCard->pos();
        QParallelAnimationGroup* pqlagSwapping = new QParallelAnimationGroup(this);
        pqlagSwapping->addAnimation(pstSelf->pstOldCard->LVL_InterCreateAnimation("pos", LVL_GAME_MOVING_MSEC, pstTarget->pstOldCard->pos(), pstSelf->pstOldCard->pos(), QEasingCurve::InOutCubic));
        pqlagSwapping->addAnimation(pstTarget->pstOldCard->LVL_InterCreateAnimation("pos", LVL_GAME_MOVING_MSEC, qpTemp, pstTarget->pstOldCard->pos(), QEasingCurve::InOutCubic));
        pqlagSwapping->start(QAbstractAnimation::DeleteWhenStopped);
        connect(pqlagSwapping, &QAbstractAnimation::finished, &qelLoop, &QEventLoop::quit);
        qelLoop.exec();
        
        //��������
        if (NULL != pstShow) {
            QAbstractAnimation* pqaaSlipping = pstShow->pstOldCard->LVL_InterCreateFilppingAnimation(LVL_GAME_FLIPPING_MSEC);
            pqaaSlipping->start(QAbstractAnimation::DeleteWhenStopped);
            connect(pqaaSlipping, &QAbstractAnimation::finished, &qelLoop, &QEventLoop::quit);
            qelLoop.exec();
        }
        break;
    }

        //Ů����
    case LVL_InterCard::countess: break;

        //����
    case LVL_InterCard::princess: pstSelf->LVL_GameKickPlayer(); break;
    }
    bIsProcessing = false;
    emit sgnNoProcessing();
}

/*
    ���ܣ�����Ϸ����ʱ���н��㣬����ʤ��
    ��������
    ����ֵ����
*/
void LVL_GameCtrller::LVL_GameCalculatePoint() {
    int asHandPoint[LVL_GAME_MAXIMUM_PLAYER_NUM];//��λ��ҵ����Ʒ���
    int asDiscardPoint[LVL_GAME_MAXIMUM_PLAYER_NUM];//��λ��ҵ����ƶѷ���
    int sWinner = -1;//ʤ��
    
    //�ҵ����Ʒ�����ߵ����
    for (int i = 0; i < sizeof(apstPlayers)/sizeof(apstPlayers[0]); i++) {
        apstPlayers[i]->pstOldCard->LVL_InterSetCardKnown(true);//չʾ�������
        if (LVL_GamePlayer::Out == apstPlayers[i]->LVL_GameGetPlayerState()) continue;//�����߲������ж�
        asHandPoint[i] = apstPlayers[i]->pstOldCard->LVL_InterGetCardType();//���������Ʒ���
        asDiscardPoint[i] = apstPlayers[i]->sDiscardPilePoint;//���������ƶѷ���
        if (-1 == sWinner || asHandPoint[sWinner] < asHandPoint[i]) sWinner = i;
        else if (asHandPoint[sWinner] == asHandPoint[i] && asDiscardPoint[sWinner] < asDiscardPoint[i]) sWinner = i;
    }
    if (0 == sWinner) QMessageBox::information(NULL, QStringLiteral("��Ϸ����"), QStringLiteral("��Ӯ���˹����ķ��ģ�"));
    else QMessageBox::information(NULL, QStringLiteral("��Ϸ����"), QString(QStringLiteral("������鱻�����ˣ����%1�����ʤ����")).arg(sWinner));
}

void LVL_GameCtrller::closeEvent(QCloseEvent *event) {
    event->accept();
    if (!bIsProcessing || pstGuesser->isVisible()) deleteLater();
    else connect(this, &LVL_GameCtrller::sgnNoProcessing, this, &QObject::deleteLater);
}

///////////////////////////////////////////////////////////////

const LVL_InterCard::LVL_InterCardType_E LVL_GameDeckManager::m_aenCardSet[LVL_GAME_TOTAL_CARD_NUM] = //���п���
{ LVL_InterCard::guard, LVL_InterCard::guard, LVL_InterCard::guard, LVL_InterCard::guard, LVL_InterCard::guard,
  LVL_InterCard::priest, LVL_InterCard::priest, LVL_InterCard::baron, LVL_InterCard::baron,
  LVL_InterCard::handmaid, LVL_InterCard::handmaid, LVL_InterCard::prince, LVL_InterCard::prince,
  LVL_InterCard::king, LVL_InterCard::countess, LVL_InterCard::princess };

/*
    ���ܣ�����������
    ���ݣ�ϴ�ƣ��Ƴ���Ҫ���ƣ�������Ļ����ʾʣ�࿨����
    ������parent-�����ڵ�ָ�룻sPlayerNum-�������
*/
LVL_GameDeckManager::LVL_GameDeckManager(QWidget *parent) : QWidget(parent), sCurCard(0) {

    //��ȡ��Ϸ����
    const LVL_GameSetting &stSettings = LVL_GameSetting::LVL_SystemGetSettings();
    int sPlayerNum = stSettings.LVL_GameReadSetting<int>("Player Number");

    //ϴ��
    for (int i = 0; i < sizeof(m_aenCardSet) / sizeof(m_aenCardSet[0]); i++) aenDrawPile[i] = m_aenCardSet[i];
    for (int i = 0; i < sizeof(m_aenCardSet) / sizeof(m_aenCardSet[0]); i++) {
        int sPos = qrand() % (LVL_GAME_TOTAL_CARD_NUM - i);
        LVL_InterCard::LVL_InterCardType_E enTemp = aenDrawPile[sPos];
        aenDrawPile[sPos] = aenDrawPile[LVL_GAME_TOTAL_CARD_NUM - i - 1];
        aenDrawPile[LVL_GAME_TOTAL_CARD_NUM - i - 1] = enTemp;
    }

    //ȡ������ʾ�ɼ�����
    for (; sCurCard < LVL_GAME_SEEN_REMOVE_CARD_NUM; sCurCard++) {
        LVL_InterCard *pstRemove = new LVL_InterCard(this, aenDrawPile[sCurCard], true,
            LVL_GAME_REMOVECARD_POS + QPoint(sCurCard * LVL_GAME_REMOVECARD_WIDTH, 0), QSize(LVL_GAME_REMOVECARD_WIDTH, LVL_GAME_REMOVECARD_HEIGHT));
        pstRemove->show();
    }

    //��ʾʣ�࿨����
    pqlRest = new QLabel(this);
    pqlRest->setGeometry(LVL_GAME_DECK_TEXT_GEO);
    QPalette qpalText = pqlRest->palette();
    qpalText.setColor(QPalette::WindowText, LVL_GAME_DECK_TEXT_COLOR);
    pqlRest->setPalette(qpalText);
    pqlRest->setFont(QFont(LVL_GAME_DECK_TEXT_FONT, LVL_GAME_DECK_TEXT_SIZE));
    pqlRest->setAlignment(Qt::AlignCenter);
}

/*
    ���ܣ�����������
    ������apstPlayers-��ȡ�Ƶ����ָ�룻pstCard-Ҫ�����ƣ�bDealUnseen-�Ƿ����������ɼ����Ƴ���
    ����ֵ��int-�ƶ��ѷ�������������ѷ��꣬����0
*/
int LVL_GameDeckManager::LVL_GameMakeDeal(LVL_GamePlayer *pstPlayer, LVL_InterCard *pstCard, bool bDealUnseen){
    if (NULL == pstPlayer || NULL == pstCard) exit(-1);
    if (false == bDealUnseen && sCurCard >= LVL_GAME_TOTAL_CARD_NUM - LVL_GAME_UNSEEN_REMOVE_CARD_NUM) return 0;//�жϿ����Ƿ���

    pstPlayer->LVL_GameSetCard(pstCard, aenDrawPile[sCurCard++]);//����
    pqlRest->setText(QString(QStringLiteral("ʣ�࿨�ƣ�%1(+%2)"))
        .arg(LVL_GAME_TOTAL_CARD_NUM - LVL_GAME_UNSEEN_REMOVE_CARD_NUM - sCurCard > 0 ? LVL_GAME_TOTAL_CARD_NUM - LVL_GAME_UNSEEN_REMOVE_CARD_NUM - sCurCard : 0)
        .arg((int)(sCurCard <= LVL_GAME_TOTAL_CARD_NUM - LVL_GAME_UNSEEN_REMOVE_CARD_NUM)));//����ʣ�࿨����
    return sCurCard;
}

///////////////////////////////////////////////////////////////

/*
    ���ܣ����Ʋ��ƽ���
    ������parent-�����ڵ�ָ�룻qpPos-����ҳ������Ͻ����ꣻ{ width,height }-����ҳ���С
*/
LVL_GameGuesser::LVL_GameGuesser(QWidget *parent, const QPoint &qpPos, const QSize &qsSize) : QWidget(parent), enGuess(LVL_InterCard::none) {
    setGeometry(QRect(qpPos, qsSize));

    //���ñ���ͼƬ
    setAutoFillBackground(true);
    QPalette qpalBackground = palette();
    qpalBackground.setBrush(QPalette::Background, QBrush(QPixmap(LVL_PIC_GUESSER).scaled(size())));
    setPalette(qpalBackground);

    //������ť���ֹ���
    QGridLayout *pqloGuesserUI = new QGridLayout(this);
    pqloGuesserUI->setMargin(LVL_GAME_GUEESER_MARGIN);
    setLayout(pqloGuesserUI);

    //����ѡ��ť
    LVL_InterBtn *pstConfirm = new LVL_InterBtn(this);
    pstConfirm->LVL_InterSetBtnPic(LVL_PIC_CONFIRM_BTN, LVL_PIC_CONFIRM_BTN + LVL_PIC_FOCUSED, LVL_PIC_CONFIRM_BTN + LVL_PIC_CLICKED);
    pstConfirm->setFixedSize(LVL_GAME_GUESSER_CONFIRM_SIZE);
    pstConfirm->hide();
    connect(pstConfirm, &QPushButton::clicked, [this] { if (LVL_InterCard::none != enGuess) emit sgnGuessFinished(enGuess); });
    pqloGuesserUI->addWidget(pstConfirm, (LVL_InterCard::CardTypeNum - 2) / LVL_GAME_GUESSER_COLUMN, (LVL_InterCard::CardTypeNum - 2) % LVL_GAME_GUESSER_COLUMN);

    //��������ѡ��
    LVL_InterBtn *apstChoice[LVL_InterCard::CardTypeNum - 2];
    for (int i = 0; i < sizeof(apstChoice)/sizeof(apstChoice[0]); i++) {
        apstChoice[i] = new LVL_InterBtn(LVL_InterCard(this, (LVL_InterCard::LVL_InterCardType_E)(i + 2)));
        apstChoice[i]->setFixedSize(apstChoice[i]->width(), apstChoice[i]->height());
        apstChoice[i]->setCheckable(true);
        apstChoice[i]->setAutoExclusive(true);
        connect(apstChoice[i], &QPushButton::toggled, [=]() { enGuess = (LVL_InterCard::LVL_InterCardType_E)(i + 2); pstConfirm->show(); });
        pqloGuesserUI->addWidget(apstChoice[i], i / LVL_GAME_GUESSER_COLUMN, i % LVL_GAME_GUESSER_COLUMN);
    }
}