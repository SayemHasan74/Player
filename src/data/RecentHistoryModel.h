#pragma once
#include <QAbstractListModel>
#include <QVariantList>
// Stable row objects while thumbnail/availability metadata arrives. Only
// actual membership/order changes reset the ten-item list.
class RecentHistoryModel final:public QAbstractListModel {
public:
    explicit RecentHistoryModel(bool pathIdentity=false):m_pathIdentity(pathIdentity){}
    enum {RecordRole=Qt::UserRole+1};
    int rowCount(const QModelIndex &parent={}) const override{return parent.isValid()?0:int(m_rows.size());}
    QVariant data(const QModelIndex &index,int role) const override{return index.isValid()&&index.row()>=0&&index.row()<m_rows.size()&&role==RecordRole?m_rows[index.row()]:QVariant();}
    QHash<int,QByteArray> roleNames() const override{return {{RecordRole,"record"}};}
    void replace(const QVariantList &rows){
        bool same=rows.size()==m_rows.size();
        const QString key=m_pathIdentity?"path":"file_key";
        if(same)for(int i=0;i<rows.size();++i)if(rows[i].toMap().value(key)!=m_rows[i].toMap().value(key)){same=false;break;}
        if(!same){beginResetModel();m_rows=rows;endResetModel();return;}
        for(int i=0;i<rows.size();++i)if(rows[i]!=m_rows[i]){m_rows[i]=rows[i];emit dataChanged(index(i),index(i),{RecordRole});}
    }
private:
    bool m_pathIdentity=false;
    QVariantList m_rows;
};
