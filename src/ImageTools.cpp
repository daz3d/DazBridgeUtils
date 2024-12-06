#define USE_DAZ_LOG 1

#include "ImageTools.h"

#if USE_DAZ_LOG
#include <dzapp.h>	
#endif

void log(QString message)
{
#if USE_DAZ_LOG
	dzApp->log(message);
#else
	printf(message.toUtf8().constData());
#endif
}


// Bitwise check if number is a power of two
bool ImageTools::isPowerOfTwo(int n)
{
    // must be positive
    if (n <= 0) return false;
   
    // bitwise AND of n with n-1
    int bitwiseCalculation = n & (n-1);
    
    // if no other bits are set to 1, then n is power of 2
    if (bitwiseCalculation == 0) return true;

    // otherwise return false
    return false;
}

// Bitwise function to find the nearest power of two
// NOTE: will fail in edge case were n is equal or greater than the largest power of 2 that can be held by an int
int ImageTools::nearestPowerOfTwo(int n)
{
    // must be positive
    if (n < 1) return 0;

    // iterate through each bit position to generate a power of 2,
    // stop when power of 2 is greater than n
    // linear time dependent upon the number of bits in n (aka, fixed iterations limited to the bit-length of an int)
    int power = 1;
    while (power < n) {
        power <<= 1;
    }

    // calculate the previous power of 2
    int prevPower = power >> 1;
    
    // return the smallest difference
    return (power - n) < (n - prevPower) ? power : prevPower;
}

void ImageTools::multiplyImageByColorMultithreaded(QImage& image, QColor color)
{
	// Crash Check
	int width = image.width();
	int height = image.height();
	int lineLength = image.bytesPerLine();
	QImage::Format pixelFormat = image.format();
	if (pixelFormat != QImage::Format_ARGB32 &&
		pixelFormat != QImage::Format_RGB32)
	{
		log(QString("WARNING: multiplyImageByColorMultithreaded(): incompatible pixel format: %1, converting to ARGB32...").arg(pixelFormat));
		image = image.convertToFormat(QImage::Format_ARGB32);
	}

	int numThreads = QThreadPool::globalInstance()->maxThreadCount();
	int step = height / numThreads;

	for (int i = 0; i < numThreads; ++i)
	{
		int startY = i * step;
		int endY = (i == numThreads - 1) ? height : startY + step;
		MultiplyImageByColorTask* task = new MultiplyImageByColorTask(&image, color, startY, endY, width, height, pixelFormat);
		QThreadPool::globalInstance()->start(task);
	}

	QThreadPool::globalInstance()->waitForDone();
}

void ImageTools::multiplyImageByStrengthMultithreaded(QImage& image, double strength)
{
	// Crash Check
	int width = image.width();
	int height = image.height();
	int lineLength = image.bytesPerLine();
	QImage::Format pixelFormat = image.format();
	if (pixelFormat != QImage::Format_ARGB32 &&
		pixelFormat != QImage::Format_RGB32)
	{
		log(QString("WARNING: multiplyImageByStrengthMultithreaded(): incompatible pixel format: %1, converting to ARGB32...").arg(pixelFormat));
		image = image.convertToFormat(QImage::Format_ARGB32);
	}

	int numThreads = QThreadPool::globalInstance()->maxThreadCount();
	int step = height / numThreads;

	for (int i = 0; i < numThreads; ++i)
	{
		int startY = i * step;
		int endY = (i == numThreads - 1) ? height : startY + step;
		MultiplyImageByStrengthTask* task = new MultiplyImageByStrengthTask(&image, strength, startY, endY, width, height, pixelFormat);
		QThreadPool::globalInstance()->start(task);
	}

	QThreadPool::globalInstance()->waitForDone();
}

QString ImageTools::colorToHexString(const QColor& color)
{
	// Extract the RGB components
	int red = color.red();
	int green = color.green();
	int blue = color.blue();

	// Convert RGB to hexadecimal and concatenate
	QString hexColor = QString("#%1%2%3")
		.arg(QString::number(red, 16).rightJustified(2, '0'))
		.arg(QString::number(green, 16).rightJustified(2, '0'))
		.arg(QString::number(blue, 16).rightJustified(2, '0'));

	return hexColor.toUpper();
}

void ImageTools::BlendImagesWithAlphaMultithreaded(QImage& imageA, const QImage& imageB, const QImage& alphaMask)
{
	// Ensure images have same dimensions and format
	int width = imageA.width();
	int height = imageA.height();
	QImage::Format pixelFormat = QImage::Format_ARGB32;

	// Convert images to consistent format if necessary
	if (imageA.format() != pixelFormat)
		imageA = imageA.convertToFormat(pixelFormat);
	QImage imageBConverted = imageB;
	if (imageBConverted.format() != pixelFormat)
		imageBConverted = imageBConverted.convertToFormat(pixelFormat);
	QImage alphaMaskConverted = alphaMask;
	if (alphaMaskConverted.format() != pixelFormat)
		alphaMaskConverted = alphaMaskConverted.convertToFormat(pixelFormat);

	// Multithreading setup
	int numThreads = QThreadPool::globalInstance()->maxThreadCount();
	int step = height / numThreads;

	for (int i = 0; i < numThreads; ++i)
	{
		int startY = i * step;
		int endY = (i == numThreads - 1) ? height : startY + step;
		BlendImagesWithAlphaTask* task = new BlendImagesWithAlphaTask(&imageA, &imageBConverted, &alphaMaskConverted, startY, endY);
		QThreadPool::globalInstance()->start(task);
	}

	QThreadPool::globalInstance()->waitForDone();
}

void ImageTools::ConvertImageLinearToRgbMultithreaded(QImage& image)
{
	// Ensure images have same dimensions and format
	int width = image.width();
	int height = image.height();
	QImage::Format pixelFormat = QImage::Format_ARGB32;

	// Convert images to consistent format if necessary
	if (image.format() != pixelFormat)
		image = image.convertToFormat(pixelFormat);

	// Multithreading setup
	int numThreads = QThreadPool::globalInstance()->maxThreadCount();
	int step = height / numThreads;

	for (int i = 0; i < numThreads; ++i)
	{
		int startY = i * step;
		int endY = (i == numThreads - 1) ? height : startY + step;
		ConvertImageLinearToRgb* task = new ConvertImageLinearToRgb(&image, startY, endY);
		QThreadPool::globalInstance()->start(task);
	}

	QThreadPool::globalInstance()->waitForDone();
}

#include <dzimagemgr.h>
#include <qfileinfo.h>
#include <dzprogress.h>

void JobReEncodeImage::performJob()
{
	DzImageMgr* imageMgr = dzApp->getImageMgr();
	QImage image = imageMgr->loadImage(m_sTextureName);
	QFileInfo fileInfo(m_sTextureName);
	QString filestem = fileInfo.fileName();
	QString fileTypeExtension = fileInfo.suffix().toLower();

	// resize image if needed
	if (m_sizeCustomImageSize != m_sizeTargetTextureSize ||
		m_bResizeTextures &&
		(image.size().width() > m_sizeTargetTextureSize.width() ||
			image.size().height() > m_sizeTargetTextureSize.height())
		)
	{
		QString sImageOperationMessage = QString(tr("Scaling: ") + filestem + " to " + QString("(%1x%2)").arg(m_sizeCustomImageSize.width()).arg(m_sizeCustomImageSize.height()));
		dzApp->log(sImageOperationMessage);
		DzProgress::setCurrentInfo(sImageOperationMessage);
		image = image.scaled(m_sizeCustomImageSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	}

	bool bSaveSuccessful = false;
	bool bCompressMore = false;
	do {
		QString sImageOperationMessage = QString(tr("Encoding: ") + filestem + " to " + fileTypeExtension);
		dzApp->log(sImageOperationMessage);
		DzProgress::setCurrentInfo(sImageOperationMessage);
		bSaveSuccessful = image.save(m_sRecompressedFilename, 0, m_nCustomEncodingQuality);
		// check if there is file size target
		int nNewFileSize = QFileInfo(m_sRecompressedFilename).size();
		bCompressMore = false;
		if (m_bRecompressIfFileSizeTooBig && nNewFileSize > m_nFileSizeThresholdToInitiateRecompression)
		{
			// decrease quality
			if (m_nCustomEncodingQuality > 10) {
				m_nCustomEncodingQuality -= 5;
				bCompressMore = true;
			}
			// decrease resolution
			if (m_sizeCustomImageSize.height() > 256 && m_sizeCustomImageSize.width() > 256) {
				m_sizeCustomImageSize = m_sizeCustomImageSize / 2;
				QString sImageOperationMessage = QString(tr("Re-Scaling: ") + filestem + " to " + QString("(%1x%2)").arg(m_sizeCustomImageSize.width()).arg(m_sizeCustomImageSize.height()));
				dzApp->log(sImageOperationMessage);
				DzProgress::setCurrentInfo(sImageOperationMessage);
				image = image.scaled(m_sizeCustomImageSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
				bCompressMore = true;
			}
		}
	} while (bCompressMore);

	m_bSuccessful = bSaveSuccessful;
}

void ImageToolsJobsManager::clearJobs()
{
	foreach(ImageToolsJob *job, m_JobPool.values())
	{
		job->deleteLater();
	}
	m_JobPool.clear();
}

#include <QtCore>
#include <QThread>
#include <QList>

bool ImageToolsJobsManager::processJobs()
{
#ifdef __SINGLE_THREAD_DEBUG
	foreach(ImageToolsJob *job, m_JobPool.values())
	{
		job->performJob();
	}
#elif defined(__APPLE__)
	std::vector<ImageToolsJob*> jobs;
	foreach (ImageToolsJob* job , m_JobPool.values())
	{
		jobs.push_back(job);
	}
	QtConcurrent::blockingMap(jobs, ImageToolsJob::StaticPerformJob);
#else
	QtConcurrent::blockingMap(m_JobPool.values(), ImageToolsJob::StaticPerformJob);
#endif

	return true;
}

bool ImageToolsJobsManager::addJob(ImageToolsJob* job)
{
	m_JobPool.insert(job->m_sJobName, job);

	return true;
}
