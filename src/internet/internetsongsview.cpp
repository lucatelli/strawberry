/*
 * Strawberry Music Player
 * Copyright 2018, Jonas Kvinge <jonas@jkvinge.net>
 *
 * Strawberry is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Strawberry is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Strawberry.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "config.h"

#include <QtGlobal>
#include <QWidget>
#include <QString>
#include <QStackedWidget>
#include <QContextMenuEvent>
#include <QSortFilterProxyModel>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QAction>

#include "core/application.h"
#include "core/iconloader.h"
#include "collection/collectionbackend.h"
#include "collection/collectionmodel.h"
#include "collection/collectionfilterwidget.h"
#include "internetservice.h"
#include "internetsongsview.h"
#include "internetcollectionview.h"
#include "ui_internetcollectionviewcontainer.h"

InternetSongsView::InternetSongsView(Application *app, InternetService *service, const QString &settings_group, const SettingsDialog::Page settings_page, QWidget *parent)
    : QWidget(parent),
      app_(app),
      service_(service),
      settings_group_(settings_group),
      settings_page_(settings_page),
      ui_(new Ui_InternetCollectionViewContainer)
      {

  ui_->setupUi(this);

  ui_->stacked->setCurrentWidget(ui_->internetcollection_page);
  ui_->view->Init(app_, service_->songs_collection_backend(), service_->songs_collection_model(), false);
  ui_->view->setModel(service_->songs_collection_sort_model());
  ui_->view->SetFilter(ui_->filter);
  ui_->filter->SetSettingsGroup(settings_group);
  ui_->filter->SetCollectionModel(service_->songs_collection_model());

  QAction *action_configure = new QAction(IconLoader::Load("configure"), tr("Configure %1...").arg(Song::TextForSource(service_->source())), this);
  connect(action_configure, SIGNAL(triggered()), SLOT(OpenSettingsDialog()));
  ui_->filter->AddMenuAction(action_configure);

  connect(ui_->view, SIGNAL(GetSongs()), SLOT(GetSongs()));
  connect(ui_->view, SIGNAL(RemoveSongs(SongList)), service_, SIGNAL(RemoveSongs(SongList)));

  connect(ui_->refresh, SIGNAL(clicked()), SLOT(GetSongs()));
  connect(ui_->close, SIGNAL(clicked()), SLOT(AbortGetSongs()));
  connect(ui_->abort, SIGNAL(clicked()), SLOT(AbortGetSongs()));
  connect(service_, SIGNAL(SongsResults(SongList, QString)), SLOT(SongsFinished(SongList, QString)));
  connect(service_, SIGNAL(SongsUpdateStatus(QString)), ui_->status, SLOT(setText(QString)));
  connect(service_, SIGNAL(SongsProgressSetMaximum(int)), ui_->progressbar, SLOT(setMaximum(int)));
  connect(service_, SIGNAL(SongsUpdateProgress(int)), ui_->progressbar, SLOT(setValue(int)));

  connect(service_->songs_collection_model(), SIGNAL(TotalArtistCountUpdated(int)), ui_->view, SLOT(TotalArtistCountUpdated(int)));
  connect(service_->songs_collection_model(), SIGNAL(TotalAlbumCountUpdated(int)), ui_->view, SLOT(TotalAlbumCountUpdated(int)));
  connect(service_->songs_collection_model(), SIGNAL(TotalSongCountUpdated(int)), ui_->view, SLOT(TotalSongCountUpdated(int)));
  connect(service_->songs_collection_model(), SIGNAL(modelAboutToBeReset()), ui_->view, SLOT(SaveFocus()));
  connect(service_->songs_collection_model(), SIGNAL(modelReset()), ui_->view, SLOT(RestoreFocus()));

  ReloadSettings();

}

InternetSongsView::~InternetSongsView() { delete ui_; }

void InternetSongsView::ReloadSettings() {

  ui_->filter->ReloadSettings();
  ui_->view->ReloadSettings();

}

void InternetSongsView::OpenSettingsDialog() {
  app_->OpenSettingsDialogAtPage(service_->settings_page());
}


void InternetSongsView::GetSongs() {

  if (!service_->authenticated() && service_->oauth()) {
    service_->ShowConfig();
    return;
  }

  ui_->status->clear();
  ui_->progressbar->show();
  ui_->abort->show();
  ui_->close->hide();
  ui_->stacked->setCurrentWidget(ui_->help_page);
  service_->GetSongs();

}

void InternetSongsView::AbortGetSongs() {

  service_->ResetSongsRequest();
  ui_->progressbar->setValue(0);
  ui_->status->clear();
  ui_->stacked->setCurrentWidget(ui_->internetcollection_page);

}

void InternetSongsView::SongsFinished(const SongList &songs, const QString &error) {

  if (songs.isEmpty() && !error.isEmpty()) {
    ui_->status->setText(error);
    ui_->progressbar->setValue(0);
    ui_->progressbar->hide();
    ui_->abort->hide();
    ui_->close->show();
  }
  else {
    service_->songs_collection_backend()->DeleteAll();
    ui_->stacked->setCurrentWidget(ui_->internetcollection_page);
    ui_->status->clear();
    service_->songs_collection_backend()->AddOrUpdateSongsAsync(songs);
  }

}
