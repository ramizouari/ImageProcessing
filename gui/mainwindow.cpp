//
// Created by ramizouari on 01/05/22.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MainWindow.h" resolved
#include <QMenuBar>
#include <QDialog>
#include <QApplication>
#include <QVBoxLayout>
#include <QFileDialog>
#include <memory>
#include <QStatusBar>
#include <iostream>
#include <QInputDialog>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QTextEdit>
#include "image/utils.h"
#include "mainwindow.h"
#include "gui/histogram/Histogram.h"
#include "image/reader.h"
#include "image/stats.h"
#include "gui/histogram/HistogramCurveView.h"
#include "image/noise/ImpulsiveNoise.h"
#include "image/noise/GaussianNoise.h"
#include "image/filter/Filter.h"
#include "image/filter/MeanBlurFilter.h"
#include "image/filter/edge/LaplacianFilter.h"
#include "image/filter/MedianFilter.h"
#include "image/filter/edge/SobelFilter.h"
#include "image/filter/gray/utils.h"
#include "options/PaddingInput.h"
#include "gui/options/GrayFilterInput.h"
#include "image/filter/edge/RobertsFilter.h"
#include "gui/options/FilterDialog.h"
#include "gui/options/ContrastDialog.h"
#include "image/filter/global/IntensityMapper.h"
#include "gui/options/MatrixInput.h"
#include "image/segmentation/OtsuSegmentation.h"
#include "image/filter/spectral/SpectralFilter.h"
#include "image/noise/SpeckleNoise.h"
#include "image/filter/GaussianBlurFilter.h"
#include "spectrum/Spectrum3DView.h"
#include "spectrum/SpectrumView.h"
#include "image/morphology/operators.h"
#include "gui/options/SpectrumMaskDialog.h"
#include "gui/options/SpectrumFilterDialog.h"

namespace GUI {
    MainWindow::MainWindow(QWidget *parent) :
            QMainWindow(parent) {

        imageLabel = new ImageView(this);

        fileMenu = new QMenu("File", this);
        auto openAction=fileMenu->addAction("Open", this, &MainWindow::openImage, QKeySequence::Open);
        auto saveAction=fileMenu->addAction("Save", this, &MainWindow::saveImage, QKeySequence::Save);
        auto saveAsAction=fileMenu->addAction("Save as", this, &MainWindow::saveImageAs, QKeySequence::SaveAs);
        fileMenu->addSeparator();
        auto quitAction=fileMenu->addAction("Quit", qApp, &QApplication::quit, QKeySequence::Quit);
        menuBar()->addMenu(fileMenu);

        editMenu = new QMenu("Edit", this);
        auto noiseMenu = editMenu->addMenu("Noise");
        auto gaussianNoiseAction=noiseMenu->addAction("Gaussian", this, &MainWindow::addGaussianNoise);
        auto saltAndPepperNoiseAction=noiseMenu->addAction("Salt and Pepper", this, &MainWindow::addSaltAndPepperNoise);
        auto speckleNoiseAction=noiseMenu->addAction("Speckle", this, &MainWindow::addSpeckleNoise);

        menuBar()->addMenu(editMenu);
        auto filterMenu = editMenu->addMenu("Filter");
        auto medianFilterAction=filterMenu->addAction("Median", this, &MainWindow::addMedianFilter);
        auto meanFilterAction=filterMenu->addAction("Mean", this, &MainWindow::addMeanBlurFilter);
        auto gaussianFilterAction=filterMenu->addAction("Gaussian", this, &MainWindow::addGaussianBlurFilter);
        auto bilateralFilterAction=filterMenu->addAction("Bilateral", this, &MainWindow::addBilateralFilter);
        auto laplacianFilterAction=filterMenu->addAction("Laplacian", this, &MainWindow::addLaplacianFilter);
        auto sobelFilterAction=filterMenu->addAction("Sobel", this, &MainWindow::addSobelFilter);
        auto robertsFilterAction=filterMenu->addAction("Roberts", this, &MainWindow::addRobertsFilter);
        auto customConvolutionFilterAction=filterMenu->addAction("Custom Convolution", this, &MainWindow::addCustomConvolutionFilter);
        editMenu->addAction("Histogram Equalization", this, &MainWindow::histogramEqualization);
        editMenu->addAction("Contrast", this,&MainWindow::mapContrast);
        editMenu->addAction("Otsu",this,&MainWindow::otsuSegmentation);
        editMenu->addAction("Gray",this,&MainWindow::grayFilter);
        editMenu->addAction("Spectral Mask", this, &MainWindow::addSpectralMask);
        editMenu->addAction("Spectral Filter", this, &MainWindow::addSpectralFilter);
        editMenu->addAction("Erosion", this, &MainWindow::addErosion);
        editMenu->addAction("Dilation", this, &MainWindow::addDilation);

        viewMenu = new QMenu("View", this);
        auto zoomInAction=viewMenu->addAction("Zoom in", imageLabel, &ImageView::zoomIn, QKeySequence::ZoomIn);
        auto zoomOutAction=viewMenu->addAction("Zoom out", imageLabel, &ImageView::zoomOut, QKeySequence::ZoomOut);


        //auto normalSizeAction=viewMenu->addAction("Normal size", imageLabel, &ImageView::normalSize);
        //auto fitToWindowAction=viewMenu->addAction("Fit to window", imageLabel, &ImageView::fitToWindow);
        auto spectrumAction=viewMenu->addAction("Spectrum", this, &MainWindow::showSpectrum);
        menuBar()->addMenu(viewMenu);

        statsMenu = new QMenu("Stats", this);
        auto histogramAction=statsMenu->addAction("Histogram", this, &MainWindow::showHistogram);
        auto histogramCurveAction=statsMenu->addAction("Histogram Curve", this, &MainWindow::showHistogramCurve);
        menuBar()->addMenu(statsMenu);


        aboutMenu = new QMenu("About", this);
        aboutMenu->addAction("About Me...", this, &MainWindow::about);
        aboutMenu->addAction("About Qt...", qApp, &QApplication::aboutQt);
        menuBar()->addMenu(aboutMenu);

        setCentralWidget(imageLabel);
        imageInformationBar = new ImageInformationBar(this);
        statusBar()->addPermanentWidget(imageInformationBar);
    }

    MainWindow::~MainWindow() {
    }

    void MainWindow::showHistogram() {
        auto dialog = new QDialog(this);
        auto layout=new QVBoxLayout(dialog);
        dialog->setLayout(layout);
        auto H=::image::stats::histogram(*imageLabel->getData());
        auto histogram = new Histogram(H,1,this);
        layout->addWidget(histogram);
        dialog->resize(800,600);
        dialog->exec();
    }

    void MainWindow::openImage() {
        QFileDialog dialog(this);
        dialog.setFileMode(QFileDialog::ExistingFile);
        dialog.filterSelected("Image (*.bpm)");
        dialog.setModal(true);
        dialog.setFocus();
        dialog.open();
        dialog.exec();

        if(dialog.selectedFiles().empty())
            return;
        auto fileName = dialog.selectedFiles().first();
        if (fileName.isEmpty()) {
            return;
        }
        image::PGMReader reader;
        imageLabel->openImage(new image::Image(reader.read(fileName.toStdString())));
        std::stringstream stream;
        stream << "Image: " << fileName.toStdString() << " (" << imageLabel->getData()->width << "x" << imageLabel->getData()->height << ")";
        statusBar()->showMessage(QString::fromStdString(stream.str()));
        imageInformationBar->update(*imageLabel->getData());
    }

    void MainWindow::saveImage() {

    }

    void MainWindow::saveImageAs() {

    }

    void MainWindow::showHistogramCurve() {
        auto dialog = new QDialog(this);
        auto layout=new QVBoxLayout(dialog);
        dialog->setLayout(layout);
        auto histogram = new HistogramCurveView(::image::stats::histogram(*imageLabel->getData()), this);
        layout->addWidget(histogram);
        dialog->resize(800,600);
        dialog->exec();
    }

    void MainWindow::addSaltAndPepperNoise() {
        auto dialog = new QInputDialog(this);
        dialog->setInputMode(QInputDialog::DoubleInput);
        dialog->setDoubleMinimum(0);
        dialog->setDoubleMaximum(1);
        dialog->setDoubleDecimals(3);
        dialog->setLabelText("Enter the impulse probability p:");
        dialog->exec();
        image::noise::ImpulsiveNoise noise(dialog->doubleValue());
        noise(*imageLabel->getData());
        imageInformationBar->update(*imageLabel->getData());
        imageLabel->updateQImage();
    }

    void MainWindow::addGaussianNoise() {
        auto dialog = new QDialog(this);
        auto dialogButton = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dialog);
        auto layout = new QFormLayout(dialog);
        auto stdInput = new QDoubleSpinBox(dialog);
        auto meanInput = new QDoubleSpinBox(dialog);
        auto stdLabel = new QLabel("Standard deviation:",dialog);
        auto meanLabel = new QLabel("Mean:",dialog);
        meanInput->setValue(0);
        stdInput->setRange(0, imageLabel->getData()->max);
        stdInput->setValue(1);
        layout->addRow(meanLabel, meanInput);
        layout->addRow(stdLabel, stdInput);
        layout->addWidget(dialogButton);
        dialog->setLayout(layout);
        connect(dialogButton, &QDialogButtonBox::accepted, [=]() {
            image::noise::GaussianNoise noise(meanInput->value(), stdInput->value());
            noise(*imageLabel->getData());
            imageInformationBar->update(*imageLabel->getData());
            imageLabel->updateQImage();
            dialog->accept();
        });
        connect(dialogButton, &QDialogButtonBox::rejected, dialog,&QDialog::reject);
        dialog->exec();

    }

    void MainWindow::addMedianFilter()
    {
        options::FilterDialog dialog(this);
        auto sizeInput = new QSpinBox(&dialog);
        dialog.addWidget("Size:", sizeInput);
        sizeInput->setRange(1, imageLabel->getData()->width);
        sizeInput->setValue(3);
        auto paddingInput =dialog.createPaddingInput();
        dialog.finalize();
        connect(&dialog, &QDialog::accepted, [=]() {
            image::filter::MedianFilter filter(sizeInput->value());
            std::unique_ptr<image::Padding> padding=options::PaddingInput::getPadding(paddingInput->currentIndex(),*imageLabel->getData());
            image::Image oldImage=*imageLabel->getData();
            if(padding)
                *imageLabel->getData() = std::move(filter.apply(*padding));
            else
                *imageLabel->getData() = std::move(filter.apply(*imageLabel->getData()));
            imageInformationBar->update(*imageLabel->getData());
            imageLabel->updateQImage();
            QString msg=tr("Median filter applied, Signal to Noise Ratio: ");
            msg+=QString::number((double) image::stats::signalToNoiseRatio(oldImage, *imageLabel->getData()));
            statusBar()->showMessage(msg);
        });
        dialog.exec();
    }

    void MainWindow::addGaussianBlurFilter() {
        options::FilterDialog dialog(this);
        QDoubleSpinBox *stdXInput = new QDoubleSpinBox(&dialog),*stdYInput=new QDoubleSpinBox(&dialog);
        QSpinBox *stdCountInput=new QSpinBox(&dialog);
        stdXInput->setRange(0, imageLabel->getData()->width);
        stdYInput->setRange(0, imageLabel->getData()->height);
        stdCountInput->setRange(1, 30);
        stdXInput->setValue(1);
        stdYInput->setValue(1);
        stdCountInput->setValue(3);
        dialog.addWidget("Standard deviation X:", stdXInput);
        dialog.addWidget("Standard Deviation Y:",stdYInput);
        dialog.addWidget("Standard Deviation Count:",stdCountInput);
        auto paddingInput =dialog.createPaddingInput();
        dialog.finalize();
        connect(&dialog, &QDialog::accepted, [=,&dialog]() {
            image::filter::GaussianBlurFilter gaussianBlurFilter(stdXInput->value(),stdYInput->value(),stdCountInput->value());
            std::unique_ptr<image::Padding> padding=options::PaddingInput::getPadding(paddingInput->currentIndex(),*imageLabel->getData());
            image::Image oldImage=*imageLabel->getData();
            if(padding)
                *imageLabel->getData() = std::move(gaussianBlurFilter.apply(*padding));
            else
                *imageLabel->getData() = std::move(gaussianBlurFilter.apply(*imageLabel->getData()));
            imageInformationBar->update(*imageLabel->getData());
            imageLabel->updateQImage();
            QString msg=tr("Gaussian blur filter applied, Signal to Noise Ratio: ");
            msg+=QString::number((double) image::stats::signalToNoiseRatio(oldImage, *imageLabel->getData()));
            statusBar()->showMessage(msg);
        });
        dialog.exec();
    }

    void MainWindow::addSpeckleNoise() {
        auto dialog = new QDialog(this);
        auto dialogButton = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dialog);
        auto layout = new QFormLayout(dialog);
        auto stdInput = new QDoubleSpinBox(dialog);
        auto stdLabel = new QLabel("Standard deviation:",dialog);
        stdInput->setRange(0, imageLabel->getData()->max);
        stdInput->setValue(.1);
        stdInput->setRange(0,0.5);
        layout->addRow(stdLabel, stdInput);
        layout->addWidget(dialogButton);
        dialog->setLayout(layout);
        connect(dialogButton, &QDialogButtonBox::accepted, [=]() {
            image::noise::SpeckleNoise noise(stdInput->value());
            noise(*imageLabel->getData());
            imageInformationBar->update(*imageLabel->getData());
            imageLabel->updateQImage();
            dialog->accept();
        });
        connect(dialogButton, &QDialogButtonBox::rejected, dialog,&QDialog::reject);
        dialog->exec();


    }

    void MainWindow::addMeanBlurFilter() {
        options::FilterDialog dialog(this);
        auto sizeInput = new QSpinBox(&dialog);
        dialog.addWidget("Size:", sizeInput);
        sizeInput->setRange(1, imageLabel->getData()->width);
        sizeInput->setValue(3);
        auto paddingInput =dialog.createPaddingInput();
        dialog.finalize();
        connect(&dialog, &QDialog::accepted, [=]() {
            image::filter::MeanBlurFilter filter(sizeInput->value());
            std::unique_ptr<image::Padding> padding=options::PaddingInput::getPadding(paddingInput->currentIndex(),*imageLabel->getData());
            image::Image oldImage=*imageLabel->getData();
            if(padding)
                *imageLabel->getData() = std::move(filter.apply(*padding));
            else
                *imageLabel->getData() = std::move(filter.apply(*imageLabel->getData()));
            imageInformationBar->update(*imageLabel->getData());
            imageLabel->updateQImage();
            QString msg=tr("Mean filter applied, Signal to Noise Ratio: ");
            msg+=QString::number((double) image::stats::signalToNoiseRatio(oldImage, *imageLabel->getData()));
            statusBar()->showMessage(msg);
        });
        dialog.exec();

    }

    void MainWindow::addBilateralFilter() {

    }

    void MainWindow::addLaplacianFilter()
    {
        options::FilterDialog dialog(this);
        auto paddingInput = dialog.createPaddingInput();
        auto grayFilterInput = dialog.createGrayFilterInput();
        auto thresholdInput=new QSpinBox(&dialog);
        dialog.addWidget("Threshold: ",thresholdInput);
        thresholdInput->setRange(0,255);
        thresholdInput->setValue(70);
        auto includeDiagonalCheckBox = new QCheckBox("Include Diagonal",&dialog);
        includeDiagonalCheckBox->setChecked(false);
        dialog.addWidget(includeDiagonalCheckBox);
        dialog.finalize();
        connect(&dialog, &QDialog::accepted, [=]() {
            image::filter::edge::LaplacianFilter filter(options::GrayFilterInput::getGrayFilter(grayFilterInput->currentIndex()),thresholdInput->value(),
                                                        includeDiagonalCheckBox->isChecked());
            std::unique_ptr<image::Padding> padding=options::PaddingInput::getPadding(paddingInput->currentIndex(),*imageLabel->getData());
            image::Image oldImage=*imageLabel->getData();
            if(padding)
                *imageLabel->getData() = std::move(filter.apply(*padding));
            else
                *imageLabel->getData() = std::move(filter.apply(*imageLabel->getData()));
            imageInformationBar->update(*imageLabel->getData());
            imageLabel->updateQImage();
        });
        dialog.exec();
    }

    void MainWindow::about() {
        QMessageBox::about(this, tr("About Image Processing"),
                           tr(R"(
<p>The <b>Image Processing</b> example demonstrates how to combine
Qt, OpenGL, and C++ to create a modern GUI application.</p><br>
Created by:
<ul>
<li> <a href="www.github.com/ramizouari"> Rami Zouari </a> </li>
<li> <a href="www.github.com/saief1999"> Saief Eddine Zneti </a> </li>
</ul>)"));

    }

    void MainWindow::histogramEqualization()
    {
        *imageLabel->getData() = std::move(image::histogramEqualization(*imageLabel->getData()));
        imageInformationBar->update(*imageLabel->getData());
        imageLabel->updateQImage();
    }

    void MainWindow::addSobelFilter() {
        options::FilterDialog dialog(this);
        auto paddingInput = dialog.createPaddingInput();
        auto grayFilterInput = dialog.createGrayFilterInput();
        auto thresholdInput=new QSpinBox(&dialog);
        dialog.addWidget("Threshold: ",thresholdInput);
        thresholdInput->setRange(0,255);
        thresholdInput->setValue(70);
        dialog.finalize();
        connect(&dialog, &QDialog::accepted, [=]() {
            image::filter::edge::SobelFilter filter(options::GrayFilterInput::getGrayFilter(grayFilterInput->currentIndex()),thresholdInput->value());
            std::unique_ptr<image::Padding> padding=options::PaddingInput::getPadding(paddingInput->currentIndex(),*imageLabel->getData());
            image::Image oldImage=*imageLabel->getData();
            if(padding)
                *imageLabel->getData() = std::move(filter.apply(*padding));
            else
                *imageLabel->getData() = std::move(filter.apply(*imageLabel->getData()));
            imageInformationBar->update(*imageLabel->getData());
            imageLabel->updateQImage();
        });
        dialog.exec();
    }

    void MainWindow::addRobertsFilter() {
        options::FilterDialog dialog(this);
        auto paddingInput = dialog.createPaddingInput();
        auto grayFilterInput = dialog.createGrayFilterInput();
        auto thresholdInput=new QSpinBox(&dialog);
        dialog.addWidget("Threshold: ",thresholdInput);
        thresholdInput->setRange(0,255);
        thresholdInput->setValue(70);
        dialog.finalize();
        connect(&dialog, &QDialog::accepted, [=]() {
            image::filter::edge::RobertsFilter filter(options::GrayFilterInput::getGrayFilter(grayFilterInput->currentIndex()),thresholdInput->value());
            std::unique_ptr<image::Padding> padding=options::PaddingInput::getPadding(paddingInput->currentIndex(),*imageLabel->getData());
            image::Image oldImage=*imageLabel->getData();
            if(padding)
                *imageLabel->getData() = std::move(filter.apply(*padding));
            else
                *imageLabel->getData() = std::move(filter.apply(*imageLabel->getData()));
            imageInformationBar->update(*imageLabel->getData());
            imageLabel->updateQImage();
        });
        dialog.exec();
    }

    void MainWindow::mapContrast() {
        options::ContrastDialog dialog(this);
        connect(&dialog,&QDialog::accepted,[&dialog,this]()
        {
            image::filter::global::IntensityMapper mapper(dialog.getPoints());
            *imageLabel->getData() = std::move(mapper.apply(*imageLabel->getData()));
            imageInformationBar->update(*imageLabel->getData());
            imageLabel->updateQImage();
        });
        dialog.exec();
    }

    void MainWindow::addCustomConvolutionFilter() {
        options::FilterDialog dialog(this);
        auto matInput=dialog.createMatrixInput();
        auto paddingInput = dialog.createPaddingInput();
        dialog.finalize();
        connect(&dialog, &QDialog::accepted, [matInput,this,paddingInput]() {
            image::filter::ConvolutionalFilter C(matInput->getConvolutionMatrix());
            std::unique_ptr<image::Padding> padding=options::PaddingInput::getPadding(paddingInput->currentIndex(),*imageLabel->getData());
            if(padding)
                *imageLabel->getData() = std::move(C.apply(*padding));
            else *imageLabel->getData() = std::move(C.apply(*imageLabel->getData()));
            imageLabel->updateQImage();
            imageInformationBar->update(*imageLabel->getData());
        });
        dialog.resize(600,600);
        dialog.exec();
    }

    void MainWindow::otsuSegmentation()
    {
        options::FilterDialog dialog(this);
        auto spinBox=dynamic_cast<QSpinBox*>(dialog.addWidget("Number of clutsers: ",new QSpinBox(&dialog)));
        auto checkBox = dynamic_cast<QCheckBox*>(dialog.addWidget(new QCheckBox(this)));
        checkBox->setText("Output binary image");
        spinBox->setRange(2,20);
        spinBox->setValue(2);
        dialog.finalize();
        connect(spinBox,&QSpinBox::valueChanged,[checkBox](int val)
        {
            checkBox->setEnabled(val==2);
        });
        connect(&dialog,&QInputDialog::accepted,[spinBox,checkBox,this]()
        {
            if(checkBox->isChecked())
            {
                image::segmentation::OtsuSegmentation otsu;
                *imageLabel->getData() = std::move(otsu.apply(*imageLabel->getData()));
            }
            else
            {
                image::segmentation::IterativeOtsuSegmentation otsu(spinBox->value());
                *imageLabel->getData() = std::move(otsu.apply(*imageLabel->getData()));
            }
            imageInformationBar->update(*imageLabel->getData());
            imageLabel->updateQImage();
        });
        dialog.exec();
    }

    void MainWindow::grayFilter() {
        options::FilterDialog dialog(this);
        auto grayFilter = dialog.createGrayFilterInput();
        dialog.finalize();
        connect(&dialog, &QDialog::accepted, [this,grayFilter]() {
            auto &filter=options::GrayFilterInput::getGrayFilter(grayFilter->currentIndex());
            *imageLabel->getData() = std::move(filter.apply(*imageLabel->getData()));
            imageLabel->updateQImage();
            imageInformationBar->update(*imageLabel->getData());
        });
        dialog.exec();
    }

    void MainWindow::addSpectralMask() {
        options::SpectrumMaskDialog dialog(*imageLabel->getData(), this);
        connect(&dialog, &QDialog::accepted, [this,&dialog]()
        {
            auto mask=dialog.getMask();
            image::Matrix maskMatrix=image::make_tensor(imageLabel->getData()->width,imageLabel->getData()->height);
            for(int i=0;i<imageLabel->getData()->width;i++) for(int j=0;j<imageLabel->getData()->height;j++)
                    maskMatrix[i][j]=(mask[i][j]^dialog.isInverted())?1:0;
            maskMatrix=image::convolution::phaseInverseTransform(maskMatrix);
            image::filter::spectral::SpectralFilter filter(maskMatrix);
            *imageLabel->getData() = std::move(filter.apply(*imageLabel->getData()));
            imageLabel->updateQImage();
            imageInformationBar->update(*imageLabel->getData());
        });
        dialog.exec();
    }

    void MainWindow::showSpectrum() {
        //auto window=GUI::spectrum::test::createGraph(*imageLabel->getData(),this);
        auto spectrum=new spectrum::SpectrumView(*imageLabel->getData());
        spectrum->setWindowTitle("Spectrum");
        spectrum->setWindowFlags(Qt::Window);
        spectrum->show();
    }

    void MainWindow::addErosion() {
        options::FilterDialog dialog(this);
        auto matInput=dialog.createMatrixInput();
        dialog.finalize();
        connect(&dialog, &QDialog::accepted, [matInput,this]() {
            *imageLabel->getData() = std::move(image::filter::morphology::operators::erosion(*imageLabel->getData(),matInput->getConvolutionMatrix()));
            imageLabel->updateQImage();
            imageInformationBar->update(*imageLabel->getData());
        });
        dialog.resize(600,600);
        dialog.exec();
    }

    void MainWindow::addDilation() {
        options::FilterDialog dialog(this);
        auto matInput=dialog.createMatrixInput();
        dialog.finalize();
        connect(&dialog, &QDialog::accepted, [matInput,this]() {
            *imageLabel->getData() = std::move(image::filter::morphology::operators::dilation(*imageLabel->getData(),matInput->getConvolutionMatrix()));
            imageLabel->updateQImage();
            imageInformationBar->update(*imageLabel->getData());
        });
        dialog.resize(600,600);
        dialog.exec();

    }

    void MainWindow::addSpectralFilter() {
        options::SpectrumFilterDialog dialog(*imageLabel->getData(), this);
        connect(&dialog, &QDialog::accepted, [this,&dialog]()
        {
            auto [X,Y] = dialog.apply();
            X=image::convolution::phaseInverseTransform(X);
            Y=image::convolution::phaseInverseTransform(Y);
            *imageLabel->getData() = image::convolution::spectrumToImage(image::convolution::imagePairToSpectrum(X,Y));
            imageLabel->updateQImage();
            imageInformationBar->update(*imageLabel->getData());
        });
        dialog.exec();
    }
} // GUI
