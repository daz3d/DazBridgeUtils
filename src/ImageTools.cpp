#include "ImageTools.h"

void multiplyImageByColorMultithreaded(QImage& image, QColor color)
{
	int height = image.height();
	int numThreads = QThreadPool::globalInstance()->maxThreadCount();
	int step = height / numThreads;

	for (int i = 0; i < numThreads; ++i)
	{
		int startY = i * step;
		int endY = (i == numThreads - 1) ? height : startY + step;
		MultiplyImageByColorTask* task = new MultiplyImageByColorTask(&image, color, startY, endY);
		QThreadPool::globalInstance()->start(task);
	}

	QThreadPool::globalInstance()->waitForDone();
}

void multiplyImageByStrengthMultithreaded(QImage& image, double strength)
{
	int height = image.height();
	int numThreads = QThreadPool::globalInstance()->maxThreadCount();
	int step = height / numThreads;

	for (int i = 0; i < numThreads; ++i)
	{
		int startY = i * step;
		int endY = (i == numThreads - 1) ? height : startY + step;
		MultiplyImageByStrengthTask* task = new MultiplyImageByStrengthTask(&image, strength, startY, endY);
		QThreadPool::globalInstance()->start(task);
	}

	QThreadPool::globalInstance()->waitForDone();
}

QString colorToHexString(const QColor& color)
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
